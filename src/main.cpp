#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <sched.h>
#include <pthread.h>

#include "queues/mpsc_queue_mutex.hpp"

void pinToCore(std::thread &thread, int core_id) {
    pthread_t handle = thread.native_handle();

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
}

int main()
{
    constexpr std::size_t N = 1'000'000;
    constexpr std::size_t CAPACITY = 4096; // 4K
    constexpr std::size_t WARMUP_ITERATIONS = 1'000'000;

    std::mutex push_mutex;

    MPSCQueueMutex<uint64_t> queue(&push_mutex, CAPACITY);
    volatile uint64_t ans = 0;

    std::atomic<bool> start_flag(false);
    std::atomic<int> warmup_done_counter(0);
    std::atomic<bool> measure_flag(false);

    std::thread producer1([&]() {
        // std::cout << "Producer thread started on core 2\n";
        while (!start_flag.load(std::memory_order_acquire)) {}

        // std::cout << "Producer thread 1 starting warmup\n";
        for (std::size_t i = 0; i < WARMUP_ITERATIONS; i++) {
            while (!queue.push(i)) {}
        }
        // std::cout << "Producer thread 1 finished warmup\n";

        warmup_done_counter.fetch_add(1, std::memory_order_release);

        // std::cout << "Producer thread 1 finished warmup, waiting for flag to start\n";
        while (!measure_flag.load(std::memory_order_acquire)) {}
        // std::cout << "Producer thread 1 starting measurement" << std::endl;

        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.push(i)) {}
        }
    });

    std::thread producer2([&]() {
        // std::cout << "Producer thread started on core 3\n";
        while (!start_flag.load(std::memory_order_acquire)) {}

        // std::cout << "Producer thread 2 starting warmup\n";
        for (std::size_t i = 0; i < WARMUP_ITERATIONS; i++) {
            while (!queue.push(i)) {}
        }
        // std::cout << "Producer thread 2 finished warmup\n";

        warmup_done_counter.fetch_add(1, std::memory_order_release);

        // std::cout << "Producer thread 2 finished warmup, waiting for flag to start\n";
        while (!measure_flag.load(std::memory_order_acquire)) {}
        // std::cout << "Producer thread 2 starting measurement" << std::endl;

        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.push(i)) {}
        }
    });

    pinToCore(producer1, 2);
    pinToCore(producer2, 3);

    std::thread consumer([&]() {
        // std::cout << "Consumer thread started on core 4\n";
        uint64_t value;
        while (!start_flag.load(std::memory_order_acquire)) {}

        // std::cout << "Consumer thread starting warmup\n";
        for (std::size_t i = 0; i < (2 * WARMUP_ITERATIONS); i++) {
            // std::cout << i << std::endl;
            while (!queue.pop(value)) {}

            ans ^= value;
        }

        // std::cout << "Consumer thread finished warmup\n";

        warmup_done_counter.fetch_add(1, std::memory_order_release);

        // std::cout << "Consumer thread finished warmup, waiting for flag to start\n";
        while (!measure_flag.load(std::memory_order_acquire)) {}

        // std::cout << "Consumer thread starting measurement" << std::endl;
        for (std::size_t i = 0; i < (2 * N); ++i) {
            // std::cout << i << std::endl;
            while (!queue.pop(value)) {}

            ans ^= value;
        }
    });

    pinToCore(consumer, 4);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    start_flag.store(true, std::memory_order_release);
    while (warmup_done_counter.load(std::memory_order_acquire) < 3) {}

    // Do things

    auto start = std::chrono::high_resolution_clock::now();

    measure_flag.store(true, std::memory_order_release);

    producer1.join();
    producer2.join();
    consumer.join();

    // Stop doing things

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();
    double throughput = (3.0 * N) / seconds;

    std::cout << ans << std::endl;
    std::cout << "Total time: " << seconds << " seconds\n";
    std::cout << "Throughput: " << throughput / 1e6 << " M ops/sec\n";
    std::cout << "Time per operation: " << (seconds / (3.0 * N)) * 1e9 << " ns/op\n";

    return 0;
}
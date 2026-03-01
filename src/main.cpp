#include <chrono>
#include <iostream>
#include <thread>
#include <sched.h>
#include <pthread.h>

#include "queues/spsc_simple_atomic.hpp"

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
    constexpr std::size_t N = 1'000'000'000;
    constexpr std::size_t CAPACITY = 4096; // 4K
    constexpr std::size_t WARMUP_ITERATIONS = 1'000'000;

    SimpleSPSCQueue<uint64_t> queue(CAPACITY);
    volatile uint64_t ans = 0;

    std::atomic<bool> start_flag(false);
    std::atomic<int> warmup_done_counter(0);
    std::atomic<bool> measure_flag(false);

    std::thread producer([&]() {
        while (!start_flag.load(std::memory_order_acquire)) {}

        for (std::size_t i = 0; i < WARMUP_ITERATIONS; i++) {
            while (!queue.push(i)) {}
        }

        warmup_done_counter.fetch_add(1, std::memory_order_release);

        while (!measure_flag.load(std::memory_order_acquire)) {}

        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.push(i)) {}
        }
    });

    pinToCore(producer, 2);

    std::thread consumer([&]() {
        uint64_t value;
        while (!start_flag.load(std::memory_order_acquire)) {}

        for (std::size_t i = 0; i < WARMUP_ITERATIONS; i++) {
            while (!queue.pop(value)) {}

            ans ^= value;
        }

        warmup_done_counter.fetch_add(1, std::memory_order_release);

        while (!measure_flag.load(std::memory_order_acquire)) {}

        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.pop(value)) {}

            ans ^= value;
        }
    });

    pinToCore(consumer, 4);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    start_flag.store(true, std::memory_order_release);
    while (warmup_done_counter.load(std::memory_order_acquire) < 2) {}

    // Do things

    auto start = std::chrono::high_resolution_clock::now();

    measure_flag.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    // Stop doing things

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    double seconds = elapsed.count();
    double throughput = (2.0 * N) / seconds;

    std::cout << ans << std::endl;
    std::cout << "Total time: " << seconds << " seconds\n";
    std::cout << "Throughput: " << throughput / 1e6 << " M ops/sec\n";
    std::cout << "Time per operation: " << (seconds / (2.0 * N)) * 1e9 << " ns/op\n";

    return 0;
}
#include <chrono>
#include <iostream>
#include <thread>

#include "queues/spsc_simple_atomic.hpp"

int main()
{
    constexpr std::size_t N = 100'000'000;

    SimpleSPSCQueue<uint64_t> queue(N);
    volatile uint64_t ans = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Do things

    std::thread producer([&]() {
        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.push(i)) {
                // busy wait if full
            }
        }
    });

    std::thread consumer([&]() {
        uint64_t value;
        for (std::size_t i = 0; i < N; ++i) {
            while (!queue.pop(value)) {
                // busy wait if empty
            }

            ans ^= value; // prevent compiler from removing work
        }
    });

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
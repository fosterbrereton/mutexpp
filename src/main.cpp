// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// spin_adaptor
#include "spin_adaptor.hpp"

/******************************************************************************/

struct cpu_clock_t {
    void reset() {
        start_m = std::clock();
    }

    double split() const {
        return 1000.0 * (std::clock() - start_m) / CLOCKS_PER_SEC;
    }

private:
    std::clock_t start_m;
};

/******************************************************************************/

struct wall_clock_t {
    void reset() {
        start_m = std::chrono::high_resolution_clock::now();
    }

    double split() const {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_m).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_m;
};

/******************************************************************************/

using clk_t = std::chrono::high_resolution_clock;
using spin_t = spin_adaptor<std::mutex>;
using spinlock_t = std::lock_guard<spin_t>;

template <typename Clock>
void worker(spin_t& mutex) {
    constexpr long milliseconds_k{1000000};

    Clock    clock;
    timespec sleep_amount = {
        0, 1 * milliseconds_k 
    };
    for (std::size_t i(0); i < 10; ++i) {
        clock.reset();
        spinlock_t lock(mutex);
        auto time = clock.split();
        std::cout << time << "\n";
        nanosleep(&sleep_amount, nullptr);
    }
}

int main(int argc, char** argv) {
    std::vector<std::thread> pool;
    spin_t                   mutex;

#if 1
    using clock_type = wall_clock_t;
#else
    using clock_type = cpu_clock_t;
#endif

    for (std::size_t i(0); i < 10; ++i) {
        pool.emplace_back(std::bind(worker<clock_type>, std::ref(mutex)));
    }

    for (auto& t : pool) {
        t.join();
    }
}

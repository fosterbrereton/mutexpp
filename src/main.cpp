// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>

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

struct thread_clock_t {
    void reset() {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_m);
    }

    double split() const {
        timespec t;

        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);

        return (t.tv_sec - start_m.tv_sec) * 1000. + ((t.tv_nsec - start_m.tv_nsec) / 1000000.);
    }

private:
    timespec start_m;
};

/******************************************************************************/

using clk_t = std::chrono::high_resolution_clock;
using spin_t = spin_adaptor<std::mutex>;
using spinlock_t = std::lock_guard<spin_t>;

/******************************************************************************/

void probe(bool did_block, spin_t::rep_t new_expected) {
    static std::size_t   count_s{0};
    static std::size_t   blocked_s{0};
    static std::ofstream output("outfile.csv");

    ++count_s;

    if (did_block)
        ++blocked_s;

    output << new_expected
           << ',' << static_cast<int>(did_block)
           << ',' << static_cast<double>(blocked_s) / count_s
           << '\n';
}

template <typename Clock>
void worker(spin_t& mutex) {
    constexpr long ms_per_ns_k{1000000};

    //Clock    clock;
    timespec sleep_amount = {
        0, 1 * ms_per_ns_k
    };

    for (std::size_t i(0); i < 100; ++i) {
        //clock.reset();
        spinlock_t lock(mutex);
        //auto time = clock.split();
        //nanosleep(&sleep_amount, nullptr);
    }
}

int main(int argc, char** argv) {
    std::vector<std::thread> pool;
    spin_t                   mutex;

    mutex.probe_m = &probe;

    for (std::size_t i(0); i < 10; ++i) {
        pool.emplace_back(std::bind(worker<wall_clock_t>, std::ref(mutex)));
    }

    for (auto& t : pool) {
        t.join();
    }
}

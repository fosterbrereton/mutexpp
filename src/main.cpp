// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <future>

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

inline void probe_log(std::ofstream& s, std::size_t& n, std::size_t& b, bool d, spin_t::rep_t e) {
    ++n;

    if (d)
        ++b;

    s << e
      << ',' << static_cast<int>(d)
      << ',' << static_cast<double>(b) / n
      << '\n';
}

template <std::size_t N>
void n_slow_probe(bool did_block, spin_t::rep_t new_expected) {
    static std::ofstream output(std::to_string(N) + "_slow.csv");
    static std::size_t n{0};
    static std::size_t b{0};

    probe_log(output, n, b, did_block, new_expected);
}

void n_slow_worker(spin_t& mutex, std::size_t i, std::size_t max) {
    bool is_slow{i < max};

    for (std::size_t i(0); i < 100; ++i) {
        spinlock_t lock(mutex);

        if (is_slow) {
            static const timespec slow_k{0, 3 * 1000000};
            nanosleep(&slow_k, nullptr);
        }
    }
}

int main(int argc, char** argv) {
    std::vector<std::future<void>> futures;
    spin_t                         slow_0_mutex;
    spin_t                         slow_1_mutex;
    spin_t                         slow_2_mutex;
    spin_t                         slow_3_mutex;
    spin_t                         slow_4_mutex;
    spin_t                         slow_5_mutex;

#if qDebug
    slow_0_mutex.probe_m = &n_slow_probe<0>;
    slow_1_mutex.probe_m = &n_slow_probe<1>;
    slow_2_mutex.probe_m = &n_slow_probe<2>;
    slow_3_mutex.probe_m = &n_slow_probe<3>;
    slow_4_mutex.probe_m = &n_slow_probe<4>;
    slow_5_mutex.probe_m = &n_slow_probe<5>;
#endif

    std::cerr << "0 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_0_mutex), i, 0));
    futures.clear();

    std::cerr << "1 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_1_mutex), i, 1));
    futures.clear();

    std::cerr << "2 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_2_mutex), i, 2));
    futures.clear();

    std::cerr << "3 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_3_mutex), i, 3));
    futures.clear();

    std::cerr << "4 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_4_mutex), i, 4));
    futures.clear();

    std::cerr << "5 slow\n";
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker, std::ref(slow_5_mutex), i, 5));
    futures.clear();
}

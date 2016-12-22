// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <future>

#define qDebug 1

// spin_adaptor
#include "spin_adaptor.hpp"

/******************************************************************************/

using spin_t = hybrid_adaptor<std::mutex>;
using spinlock_t = std::lock_guard<spin_t>;

/******************************************************************************/

inline void probe_log(std::ofstream& s, std::size_t& n, std::size_t& b, bool d, predictor_t::rep_t e) {
    ++n;

    if (d)
        ++b;

    s << e
      << ',' << static_cast<int>(d)
      << ',' << static_cast<double>(b) / n
      << '\n';
}

template <std::size_t N>
void n_slow_probe(bool did_block, predictor_t::rep_t new_p) {
    static std::ofstream output(std::to_string(N) + "_slow.csv");
    static std::size_t n{0};
    static std::size_t b{0};

    probe_log(output, n, b, did_block, new_p);
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

    slow_0_mutex._probe = &n_slow_probe<0>;
    slow_1_mutex._probe = &n_slow_probe<1>;
    slow_2_mutex._probe = &n_slow_probe<2>;
    slow_3_mutex._probe = &n_slow_probe<3>;
    slow_4_mutex._probe = &n_slow_probe<4>;
    slow_5_mutex._probe = &n_slow_probe<5>;

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

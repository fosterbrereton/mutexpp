// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <future>

#define MUTEXPP_ENABLE_PROBE 1

// mutexpp
#include "mutexpp.hpp"

/******************************************************************************/

using namespace mutexpp;

/******************************************************************************/

template <typename T>
const char* pretty_type();

template <>
const char* pretty_type<hybrid_spin_mutex_t>() { return "hybrid_spin_mutex_t"; }

template <>
const char* pretty_type<averse_hybrid_mutex_t>() { return "averse_hybrid_mutex_t"; }

/******************************************************************************/

inline void probe_log(std::ofstream& out,
                      std::size_t&   n_total,
                      std::size_t&   n_blocked,
                      bool           did_block,
                      tp_diff_t      new_p,
                      tp_diff_t      new_b) {
    ++n_total;

    if (did_block)
        ++n_blocked;

    out << static_cast<int>(did_block)
        << ',' << new_p
        << ',' << new_b
        << '\n';
}

template <typename Mutex, std::size_t N>
void n_slow_probe(bool      did_block,
                  tp_diff_t new_p,
                  tp_diff_t new_b) {
    static std::ofstream out_s(pretty_type<Mutex>() +
                             std::string("_") +
                             std::to_string(N) +
                             "_slow.csv");
    static std::size_t   n_total_s{0};
    static std::size_t   n_blocked_s{0};
    static bool          first_s{true};

    if (first_s) {
        first_s = false;

        out_s << "blok_phas,time_spin,time_blok\n";
    }

    probe_log(out_s, n_total_s, n_blocked_s, did_block, new_p, new_b);
}

template <typename Mutex>
void n_slow_worker(Mutex& mutex, std::size_t i, std::size_t max) {
    bool is_slow{i < max};

    for (std::size_t i(0); i < 100; ++i) {
        std::lock_guard<Mutex> lock(mutex);

        if (is_slow) {
            static const timespec slow_k{0, 3 * 1000000};
            nanosleep(&slow_k, nullptr);
        }
    }
}

template <typename Mutex, std::size_t N>
void test_mutex_n_slow() {
    std::vector<std::future<void>> futures;
    Mutex                          mutex;

    mutex._probe = &n_slow_probe<Mutex, N>;
 
    std::cerr << pretty_type<Mutex>() << " " << N << " slow\n";
 
    for (std::size_t i(0); i < 5; ++i)
        futures.emplace_back(std::async(n_slow_worker<Mutex>, std::ref(mutex), i, N));
}

template <typename Mutex>
void text_mutex_type() {
    test_mutex_n_slow<Mutex, 0>();
    test_mutex_n_slow<Mutex, 1>();
    test_mutex_n_slow<Mutex, 2>();
    test_mutex_n_slow<Mutex, 3>();
    test_mutex_n_slow<Mutex, 4>();
    test_mutex_n_slow<Mutex, 5>();
}

int main(int argc, char** argv) {
    text_mutex_type<hybrid_spin_mutex_t>();
    text_mutex_type<averse_hybrid_mutex_t>();
}

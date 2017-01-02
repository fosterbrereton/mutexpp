/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

// stdc++
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <map>

#define MUTEXPP_ENABLE_PROBE 1

// mutexpp
#include "mutexpp.hpp"

// application
#include "analysis.hpp"

/******************************************************************************/

using namespace mutexpp;

/******************************************************************************/

template <typename T>
std::string pretty_type();

template <>
std::string pretty_type<std::mutex>() { return "stdmutex"; }

template <>
std::string pretty_type<hybrid_spin_mutex_t>() { return "hybrid_spin_mutex_t"; }

template <>
std::string pretty_type<averse_hybrid_mutex_t>() { return "averse_hybrid_mutex_t"; }

/******************************************************************************/

typedef std::pair<tp_t, tp_t> time_pair_t;

/******************************************************************************/

inline void probe_log(std::ofstream& out,
                      std::size_t&   n_total,
                      std::size_t&   n_blocked,
                      bool           did_block,
                      duration_t     new_p,
                      duration_t     new_b) {
    ++n_total;

    if (did_block)
        ++n_blocked;

    out << static_cast<int>(did_block)
        << ',' << new_p.count()
        << ',' << new_b.count()
        << '\n';
}

template <typename Mutex, std::size_t N>
void n_slow_probe(bool       did_block,
                  duration_t new_p,
                  duration_t new_b) {
    static std::ofstream out_s(pretty_type<Mutex>() + "_" +
                               std::to_string(N) +
                               "_slow.csv");
    static std::size_t   n_total_s{0};
    static std::size_t   n_blocked_s{0};
    static bool          first_s{true};

    if (first_s) {
        first_s = false;

        out_s << "blok_phas,spin (us),block (us)\n";
    }

    probe_log(out_s, n_total_s, n_blocked_s, did_block, new_p, new_b);
}

template <typename Mutex>
void n_slow_worker(Mutex& mutex, std::size_t i, std::size_t max) {
    constexpr std::size_t count_k = 100;

    bool                     is_slow{i < max};
    // std::vector<time_pair_t> time_pairs(count_k, time_pair_t());

    for (std::size_t i(0); i < count_k; ++i) {
        // time_pairs[i].first = mutexpp::clock_t::now();
        std::lock_guard<Mutex> lock(mutex);
        // time_pairs[i].second = mutexpp::clock_t::now();

        if (is_slow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }

#if 0
    std::vector<double> times;

    for (const auto& pair : time_pairs) {
        times.push_back(std::chrono::duration_cast<mutexpp::duration_t>(pair.second - pair.first).count());
    }

    std::sort(times.begin(), times.end());

    std::ofstream ttl_out(pretty_type<Mutex>() + "_" +
                          std::string("worker_") +
                          std::to_string(max) + "_" +
                          std::to_string(i) +
                          "_ttl.csv");

    ttl_out << "dur (us)\n";

    for (const auto& time : times) {
        ttl_out << time << "\n";
    }
#endif
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

template <typename Mutex>
void mutex_compare_map_insert_specific() {
    std::vector<double> times;

    for (std::size_t i(0); i < 100; ++i) {
        Mutex                               mutex;
        std::vector<std::future<void>>      futures;
        std::map<std::string, std::string>  map;
        tp_t                                start = mutexpp::clock_t::now();

        for (std::size_t i(0); i < 5; ++i) {
            futures.emplace_back(std::async([&mutex, &map]() {
                for (std::size_t i(0); i < 1000; ++i) {
                    std::string key = std::to_string(std::rand());
                    std::string value = std::to_string(std::rand());

                    std::lock_guard<Mutex> lock(mutex);
                    map[key] = value;
                }
            }));
        }

        futures.clear();

        tp_t end = mutexpp::clock_t::now();

        times.push_back(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start).count());
    }

    normal_analysis_t results = normal_analysis(times);

    std::cerr << "  "
              << pretty_type<Mutex>()
              << ": "
              << results
              << '\n';
}

void mutex_compare_map_insert() {
    std::cerr << "mutex_compare_map_insert:\n";

    mutex_compare_map_insert_specific<std::mutex>();
    mutex_compare_map_insert_specific<hybrid_spin_mutex_t>();
    mutex_compare_map_insert_specific<averse_hybrid_mutex_t>();
}

void mutex_compare() {
    mutex_compare_map_insert();
}

int main(int argc, char** argv) {
    std::srand(std::time(nullptr));

    //text_mutex_type<hybrid_spin_mutex_t>();
    //text_mutex_type<averse_hybrid_mutex_t>();

    mutex_compare();
}

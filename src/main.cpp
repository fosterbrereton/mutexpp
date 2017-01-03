/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

// stdc++
#include <fstream>
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

static const std::size_t thread_exact_k = std::thread::hardware_concurrency();
static const std::size_t thread_under_k = thread_exact_k / 2;
static const std::size_t thread_over_k = thread_exact_k * 2;

/******************************************************************************/

template <typename T>
std::string pretty_type();

template <>
std::string pretty_type<std::mutex>() { return "std"; }

template <>
std::string pretty_type<spin_mutex_t>() { return "spin"; }

template <>
std::string pretty_type<adaptive_spin_mutex_t>() { return "adaptive_spin"; }

template <>
std::string pretty_type<adaptive_block_mutex_t>() { return "adaptive_block"; }

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

/******************************************************************************/

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

/******************************************************************************/

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

/******************************************************************************/

template <typename Mutex, std::size_t N>
void test_mutex_n_slow() {
    std::vector<std::thread> pool;
    Mutex                    mutex;

#if MUTEXPP_ENABLE_PROBE
    mutex._probe = &n_slow_probe<Mutex, N>;
#endif

    std::cerr << pretty_type<Mutex>() << " " << N << " slow\n";
 
    for (std::size_t i(0); i < 5; ++i)
        pool.emplace_back(n_slow_worker<Mutex>, std::ref(mutex), i, N);

    for (auto& t : pool)
        t.join();
}

/******************************************************************************/

template <typename Mutex>
void mutex_benchmark_specific() {
    test_mutex_n_slow<Mutex, 0>();
    test_mutex_n_slow<Mutex, 1>();
    test_mutex_n_slow<Mutex, 2>();
    test_mutex_n_slow<Mutex, 3>();
    test_mutex_n_slow<Mutex, 4>();
    test_mutex_n_slow<Mutex, 5>();
}

/******************************************************************************/

template <typename Mutex>
struct map_insert_test {
    using mutex_type = Mutex;

    void run_once(mutex_type& mutex, std::size_t, std::size_t) {
        std::string key = std::to_string(std::rand());
        std::string value = std::to_string(std::rand());

        std::lock_guard<mutex_type> lock(mutex);
        map_m[key] = value;
    }

    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <typename Mutex>
struct map_search_test {
    using mutex_type = Mutex;

    map_search_test() {
        for (std::size_t i(0); i < 100000; ++i) {
            std::string key = std::to_string(std::rand());
            std::string value = std::to_string(std::rand());

            map_m[key] = value;
        }
    }

    void run_once(mutex_type& mutex, std::size_t, std::size_t) {
        std::string key = std::to_string(std::rand());

        std::lock_guard<Mutex> lock(mutex);
        (void)map_m.find(key);
    }

    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <typename Mutex>
struct map_hybrid_test {
    using mutex_type = Mutex;

    void run_once(mutex_type& mutex, std::size_t thread_n, std::size_t thread_i) {
        std::string key = std::to_string(std::rand());

        if (thread_i == 0) {
            std::string value = std::to_string(std::rand());

            std::lock_guard<Mutex> lock(mutex);
            map_m[key] = value;
        } else {
            std::lock_guard<Mutex> lock(mutex);
            (void)map_m.find(key);
        }
    }

    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <typename Test>
void run_test_instance(std::size_t thread_count) {
    using mutex_type = typename Test::mutex_type;

    std::vector<double> wall_times;
    std::vector<double> cpu_times;
    Test                test;

    for (std::size_t test_i(0); test_i < 100; ++test_i) {
        mutex_type               mutex;
        std::vector<std::thread> pool;
        tp_t                     wall_start = mutexpp::clock_t::now();
        std::clock_t             cpu_start = std::clock();

        for (std::size_t thread_i(0); thread_i < thread_count; ++thread_i) {
            pool.emplace_back([&mutex, &test, thread_count, thread_i]() {
                for (std::size_t inner_i(0); inner_i < 1000; ++inner_i) {
                    test.run_once(mutex, thread_count, thread_i);
                }
            });
        }

        for (auto& thread : pool)
            thread.join();

        tp_t         wall_end = mutexpp::clock_t::now();
        std::clock_t cpu_end = std::clock();

        wall_times.push_back(std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(wall_end - wall_start).count());
        // REVISIT (fbrereto) : CLOCKS_PER_SEC is microsecond precision under
        // macOS. I doubt it is the same everywhere.
        cpu_times.push_back(1000. * (cpu_end - cpu_start) / CLOCKS_PER_SEC);
    }

    std::cerr << "  "
              << pretty_type<mutex_type>() << "/wal"
              << ": "
              << normal_analysis(wall_times)
              << '\n';

    std::cerr << "  "
              << pretty_type<mutex_type>() << "/cpu"
              << ": "
              << normal_analysis(cpu_times)
              << '\n';
}

/******************************************************************************/

template <template <typename> class Test>
void run_test_aggregate(const char* name, std::size_t thread_count) {
    std::cerr << name << ' ' << thread_count << '/' << thread_exact_k << '\n';

    run_test_instance<Test<std::mutex>>(thread_count);
    run_test_instance<Test<spin_mutex_t>>(thread_count);
    run_test_instance<Test<adaptive_spin_mutex_t>>(thread_count);
    run_test_instance<Test<adaptive_block_mutex_t>>(thread_count);
}

/******************************************************************************/

template <template <typename> class Test>
void run_test_aggregate(const char* name) {
    run_test_aggregate<Test>(name, thread_under_k);
    run_test_aggregate<Test>(name, thread_exact_k);
    run_test_aggregate<Test>(name, thread_over_k);
}

/******************************************************************************/

void mutex_compare() {
    run_test_aggregate<map_insert_test>("map_insert_test");
    run_test_aggregate<map_search_test>("map_search_test");
    run_test_aggregate<map_hybrid_test>("map_hybrid_test");
}

/******************************************************************************/
#if MUTEXPP_ENABLE_PROBE
void mutex_benchmark() {
    mutex_benchmark_specific<spin_mutex_t>();
    mutex_benchmark_specific<adaptive_spin_mutex_t>();
    mutex_benchmark_specific<adaptive_block_mutex_t>();
}
#endif
/******************************************************************************/

int main(int argc, char** argv) {
    std::srand(std::time(nullptr));

#if MUTEXPP_ENABLE_PROBE
    mutex_benchmark();
#endif
    mutex_compare();
}

/******************************************************************************/

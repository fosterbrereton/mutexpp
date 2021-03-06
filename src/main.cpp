/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

// stdc++
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <map>

// tbb
#include <tbb/mutex.h>
#include <tbb/spin_mutex.h>

#define MUTEXPP_SERIAL_QUEUE_IMPL 0 // portable

#define MUTEXPP_ENABLE_PROBE 0

// mutexpp
#include "mutexpp.hpp"
#include "serial_queue.hpp"

// application
#include "analysis.hpp"

/******************************************************************************/

using namespace mutexpp;

using std::chrono::duration_cast;
using std::chrono::duration;

/******************************************************************************/

static const std::size_t thread_exact_k = std::thread::hardware_concurrency();
static const std::size_t thread_under_k = thread_exact_k / 2;
static const std::size_t thread_over_k = thread_exact_k * 2;

/******************************************************************************/

template <typename T>
std::string pretty_type();

template <>
std::string pretty_type<tbb::mutex>() { return "tbb/mutex"; }

template <>
std::string pretty_type<tbb::spin_mutex>() { return "tbb/spin"; }

template <>
std::string pretty_type<spin_mutex_t>() { return "spin"; }

template <>
std::string pretty_type<adaptive_spin_mutex_t>() { return "adaptive spin"; }

template <>
std::string pretty_type<adaptive_block_mutex_t>() { return "adaptive block"; }

/******************************************************************************/

#if MUTEXPP_ENABLE_PROBE

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

    bool is_slow{i < max};

    for (std::size_t i(0); i < count_k; ++i) {
        std::lock_guard<Mutex> lock(mutex);

        if (is_slow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
}

/******************************************************************************/

template <typename Mutex, std::size_t N>
void test_mutex_n_slow() {
    std::vector<std::thread> pool;
    Mutex                    mutex;

    mutex._probe = &n_slow_probe<Mutex, N>;

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

void mutex_benchmark() {
    mutex_benchmark_specific<spin_mutex_t>();
    mutex_benchmark_specific<tbb::spin_mutex>();
    mutex_benchmark_specific<adaptive_spin_mutex_t>();
    mutex_benchmark_specific<adaptive_block_mutex_t>();
}

/******************************************************************************/

#endif // MUTEXPP_ENABLE_PROBE

/******************************************************************************/

struct map_insert_test_serial_t {
    std::future<void> run_once(serial_queue_t& q, std::size_t) {
        std::string key = std::to_string(std::rand());
        std::string value = std::to_string(std::rand());

        return q.async([this, key, value](){
            map_m[key] = value;
        });
    }

    std::map<std::string, std::string> map_m;
};

struct map_search_test_serial_t {
    explicit map_search_test_serial_t() {
        for (std::size_t i(0); i < 100000; ++i) {
            std::string key = std::to_string(std::rand());
            std::string value = std::to_string(std::rand());

            map_m[key] = value;
        }
    }

    std::future<void> run_once(serial_queue_t& q, std::size_t) {
        std::string key = std::to_string(std::rand());

        return q.async([this, key](){
            (void)map_m.find(key);
        });
    }

    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <typename Mutex>
struct map_insert_test {
    using mutex_type = Mutex;

    explicit map_insert_test(std::size_t) { }

    void run_once(mutex_type& mutex, std::size_t) {
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

    explicit map_search_test(std::size_t) {
        for (std::size_t i(0); i < 100000; ++i) {
            std::string key = std::to_string(std::rand());
            std::string value = std::to_string(std::rand());

            map_m[key] = value;
        }
    }

    void run_once(mutex_type& mutex, std::size_t) {
        std::string key = std::to_string(std::rand());

        std::lock_guard<Mutex> lock(mutex);
        (void)map_m.find(key);
    }

    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <std::size_t SlowThreshold, typename Mutex>
struct map_hybrid_test {
    using mutex_type = Mutex;

    explicit map_hybrid_test(std::size_t thread_count) :
        write_group_m(static_cast<std::size_t>((std::max)(thread_count * (SlowThreshold / 100.), 0.)))
    { }

    void run_once(mutex_type& mutex, std::size_t thread_i) {
        std::string key = std::to_string(std::rand());

        if (thread_i <= write_group_m) {
            std::string value = std::to_string(std::rand());

            std::lock_guard<Mutex> lock(mutex);
            map_m[key] = value;
        } else { // read group
            std::lock_guard<Mutex> lock(mutex);
            (void)map_m.find(key);
        }
    }

    const std::size_t                  write_group_m;
    std::map<std::string, std::string> map_m;
};

/******************************************************************************/

template <typename Test>
void run_test_instance(std::size_t thread_count, std::ostream& out) {
    using mutex_type = typename Test::mutex_type;

    std::vector<double> wall_times;
    std::vector<double> cpu_times;
    Test                test(thread_count);

    for (std::size_t test_i(0); test_i < 100; ++test_i) {
        mutex_type               mutex;
        std::vector<std::thread> pool;
        std::atomic<bool>        go{false};

        for (std::size_t thread_i(0); thread_i < thread_count; ++thread_i) {
            pool.emplace_back([&mutex, &test, &go, thread_i]() {
                while (!go); // spin here until we go.

                for (std::size_t inner_i(0); inner_i < 1000; ++inner_i) {
                    test.run_once(mutex, thread_i);
                }
            });
        }

        tp_t         wall_start = mutexpp::clock_t::now();
        std::clock_t cpu_start = std::clock();

        go = true;

        for (auto& thread : pool)
            thread.join();

        tp_t         wall_end = mutexpp::clock_t::now();
        std::clock_t cpu_end = std::clock();

        wall_times.push_back(duration_cast<duration<double, std::milli>>(wall_end - wall_start).count());

        {
        using std::clock_t; // *shakes fist at msvc*
        constexpr double cpms_k{CLOCKS_PER_SEC/1000.}; // clocks per millisecond
        cpu_times.push_back((cpu_end - cpu_start) / cpms_k);
        }
    }

    out << pretty_type<mutex_type>() << " wall"
        << ","
        << normal_analysis(wall_times)
        << '\n';

    out << pretty_type<mutex_type>() << " cpu"
        << ","
        << normal_analysis(cpu_times)
        << '\n';
}

/******************************************************************************/

template <typename Test>
void run_test_instance_serial(std::ostream& out) {
    constexpr std::size_t test_count_k{100};
    constexpr std::size_t inner_count_k{1000};

    std::vector<double> wall_times;
    std::vector<double> cpu_times;
    Test                test;
    serial_queue_t      q;

    for (std::size_t test_i(0); test_i < test_count_k; ++test_i) {
        tp_t                           wall_start = mutexpp::clock_t::now();
        std::clock_t                   cpu_start = std::clock();
        std::vector<std::future<void>> futures(inner_count_k);

        for (std::size_t inner_i(0); inner_i < inner_count_k; ++inner_i) {
            futures[inner_i] = test.run_once(q, inner_i);
        }

        for (auto& future : futures)
            future.get();

        tp_t         wall_end = mutexpp::clock_t::now();
        std::clock_t cpu_end = std::clock();

        wall_times.push_back(duration_cast<duration<double, std::milli>>(wall_end - wall_start).count());

        {
        using std::clock_t; // *shakes fist at msvc*
        constexpr double cpms_k{CLOCKS_PER_SEC/1000.}; // clocks per millisecond
        cpu_times.push_back((cpu_end - cpu_start) / cpms_k);
        }
    }

    out << "serial wall,"
        << normal_analysis(wall_times)
        << '\n';

    out << "serial cpu,"
        << normal_analysis(cpu_times)
        << '\n';
}

/******************************************************************************/

template <template <typename> class Test>
void run_test_aggregate(const char* name, std::size_t thread_count, std::ostream& out) {
    out << name << ' ' << thread_count << '/' << thread_exact_k << '\n';

    run_test_instance<Test<tbb::mutex>>(thread_count, out);
    run_test_instance<Test<tbb::spin_mutex>>(thread_count, out);
    run_test_instance<Test<spin_mutex_t>>(thread_count, out);
    run_test_instance<Test<adaptive_spin_mutex_t>>(thread_count, out);
    run_test_instance<Test<adaptive_block_mutex_t>>(thread_count, out);
}

/******************************************************************************/

template <template <typename> class Test>
void run_test_aggregate(const char* name, std::ostream& out) {
    out << "name,";
    normal_analysis_header(out);

    run_test_aggregate<Test>(name, thread_under_k, out);
    run_test_aggregate<Test>(name, thread_exact_k, out);
    run_test_aggregate<Test>(name, thread_over_k, out);

    out.flush();
}

/******************************************************************************/

template <class Mutex>
using hybrid_25 = map_hybrid_test<25, Mutex>;
template <class Mutex>
using hybrid_50 = map_hybrid_test<50, Mutex>;
template <class Mutex>
using hybrid_75 = map_hybrid_test<75, Mutex>;

void mutex_compare() {
    run_test_aggregate<map_insert_test>("map_insert_test", std::cerr);
    run_test_aggregate<map_search_test>("map_search_test", std::cerr);
    run_test_aggregate<hybrid_25>("map_hybrid_25_test", std::cerr);
    run_test_aggregate<hybrid_50>("map_hybrid_50_test", std::cerr);
    run_test_aggregate<hybrid_75>("map_hybrid_75_test", std::cerr);
}

/******************************************************************************/

template <typename Test>
void run_test_comprehensive_instance(std::ostream& out) {
    static std::size_t max_threads_k = std::thread::hardware_concurrency() * 2;

    out << "name,";
    normal_analysis_header(out);

    for (std::size_t i(1); i <= max_threads_k; ++i) {
        run_test_instance<Test>(i, out);
        out.flush();
    }
}

/******************************************************************************/

template <template <typename> class Test>
void run_test_comprehensive(const char* name, std::ostream& out) {
    out << name << '\n';

    run_test_comprehensive_instance<Test<tbb::mutex>>(out);
    run_test_comprehensive_instance<Test<spin_mutex_t>>(out);
    run_test_comprehensive_instance<Test<adaptive_spin_mutex_t>>(out);
    run_test_comprehensive_instance<Test<adaptive_block_mutex_t>>(out);
}

/******************************************************************************/

void mutex_comprehensive() {
    std::ofstream out("comprehensive.csv");

    run_test_comprehensive<map_insert_test>("map_insert_test", out);
    run_test_instance_serial<map_insert_test_serial_t>(out);
    run_test_instance_serial<map_search_test_serial_t>(out);
}

/******************************************************************************/

void serial_queue_test() {
    std::ofstream out("serial_queue.csv");

    out << "name,";
    normal_analysis_header(out);
    run_test_instance_serial<map_insert_test_serial_t>(out);
    run_test_instance_serial<map_search_test_serial_t>(out);
}

/******************************************************************************/

void serial_wrapper_test() {
    typedef mutexpp::serial_wrapper<std::map<std::string, std::string>> serial_map_t;

    tp_t start = mutexpp::clock_t::now();

    /* serial map test */ {
        serial_map_t serial_map;

        for (std::size_t i(0); i < 10000; ++i) {
            serial_map([i](decltype(serial_map)::value_type& map){
                std::string key = std::to_string(i);
                map.emplace(key, key + "_value");
            });
        }

        for (std::size_t i(0); i < 10000; ++i) {
            auto result = serial_map([](serial_map_t::value_type& map){
                auto iter = map.find("42");
                return (iter == map.end()) ? "<END>" : iter->second;
            });
            if (result.get() != "42_value") throw std::runtime_error("unexpected not found");

            result = serial_map([](serial_map_t::value_type& map){
                auto iter = map.find("-1");
                return (iter == map.end()) ? "<END>" : iter->second;
            });
            if (result.get() != "<END>") throw std::runtime_error("unexpected found");
        }
    }

    tp_t split = mutexpp::clock_t::now();

    /* non-serial map test */ {
        typedef std::unique_lock<std::mutex> lock_t;

        serial_map_t::value_type map;
        std::mutex               mutex;

        for (std::size_t i(0); i < 10000; ++i) {
            // std::async would return a future<void> here, which blocks on
            // destruction. As such we don't need to wrap it, other than to
            // account for the std::async call overhead.
            lock_t lock(mutex);
            std::string key = std::to_string(i);
            map.emplace(key, key + "_value");
        }

        for (std::size_t i(0); i < 10000; ++i) {
            auto result = std::async([&mutex, &map]() {
                lock_t lock(mutex);
                auto iter = map.find("42");
                return (iter == map.end()) ? "<END>" : iter->second;
            });
            if (result.get() != "42_value") throw std::runtime_error("unexpected not found");

            result = std::async([&mutex, &map]() {
                lock_t lock(mutex);
                auto iter = map.find("-1");
                return (iter == map.end()) ? "<END>" : iter->second;
            });
            if (result.get() != "<END>") throw std::runtime_error("unexpected found");
        }
    }

    tp_t end = mutexpp::clock_t::now();

    std::cerr << "   serial: " << duration_cast<duration<double, std::milli>>(split - start).count() << '\n';
    std::cerr << "nonserial: " << duration_cast<duration<double, std::milli>>(end - split).count() << '\n';
}

/******************************************************************************/

int main(int argc, char** argv) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

#if MUTEXPP_ENABLE_PROBE
    mutex_benchmark();
#endif
    //mutex_compare();

    //mutex_comprehensive();

    serial_queue_test();

    serial_wrapper_test();
}

/******************************************************************************/

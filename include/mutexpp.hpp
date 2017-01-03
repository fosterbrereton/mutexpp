/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef MUTEXPP_HPP__
#define MUTEXPP_HPP__

/******************************************************************************/

#include <atomic>
#include <chrono>

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

using clock_t = std::chrono::high_resolution_clock;
using tp_t = clock_t::time_point;
using diff_t = decltype((std::declval<tp_t>() - std::declval<tp_t>()).count());
using duration_t = std::chrono::duration<double, std::micro>;
#if MUTEXPP_ENABLE_PROBE
using probe_t = void (*)(bool did_block, duration_t new_p, duration_t new_b);
#endif

/******************************************************************************/

class spin_mutex_t {
private:
    std::atomic_flag _lock{ATOMIC_FLAG_INIT};

public:
#if MUTEXPP_ENABLE_PROBE
    probe_t _probe{nullptr};
#endif

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);
    }

    void lock() {
#if MUTEXPP_ENABLE_PROBE
        tp_t start = clock_t::now();
#endif

        while (!try_lock()) { }

#if MUTEXPP_ENABLE_PROBE
        if (_probe) {
            _probe(false,
                   std::chrono::duration_cast<duration_t>(clock_t::now() - start),
                   std::chrono::duration_cast<duration_t>(tp_t::duration(0)));
        }
#endif
    }

    void unlock() {
        _lock.clear(std::memory_order_release);
    }
};

/******************************************************************************/

class adaptive_spin_mutex_t {
private:
    std::atomic_flag    _lock{ATOMIC_FLAG_INIT};
    std::atomic<diff_t> _spin_pred{0};

public:
#if MUTEXPP_ENABLE_PROBE
    probe_t _probe{nullptr};
#endif

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);
    }

    void lock() {
#if MUTEXPP_ENABLE_PROBE
        bool   did_block{false};
#endif
        tp_t   spin_start{clock_t::now()};
        diff_t spin_meas{0};

        while (!try_lock()) {
            spin_meas = (clock_t::now() - spin_start).count();

            if (spin_meas < _spin_pred * 2)
                continue;

            // REVISIT (fbrereto) : Iff sleep_for is guaranteed to block even when the
            // duration is 0, then std::chrono::nanoseconds(0) will suffice here.
            std::this_thread::sleep_for(std::chrono::microseconds(1));

#if MUTEXPP_ENABLE_PROBE
            did_block = true;
#endif
        }

        _spin_pred += (spin_meas - _spin_pred) / 8;

#if MUTEXPP_ENABLE_PROBE
        if (_probe) {
            _probe(did_block,
                   std::chrono::duration_cast<duration_t>(tp_t::duration(_spin_pred)),
                   std::chrono::duration_cast<duration_t>(tp_t::duration(0)));
        }
#endif
    }

    void unlock() {
        _lock.clear(std::memory_order_release);
    }
};

/******************************************************************************/

class adaptive_block_mutex_t {
private:
    std::atomic_flag    _lock{ATOMIC_FLAG_INIT};
    std::atomic<diff_t> _lock_pred{0};
    tp_t                _lock_start;

public:
#if MUTEXPP_ENABLE_PROBE
    probe_t _probe;
#endif

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);
    }

    void lock() {
#if MUTEXPP_ENABLE_PROBE
        bool   did_block{false};
#endif

        while (!try_lock()) {
            std::this_thread::sleep_for(tp_t::duration(_lock_pred));

#if MUTEXPP_ENABLE_PROBE
            did_block = true;
#endif
        }

#if MUTEXPP_ENABLE_PROBE
        if (_probe)
            _probe(did_block,
                   std::chrono::duration_cast<duration_t>(tp_t::duration(0)),
                   std::chrono::duration_cast<duration_t>(tp_t::duration(_lock_pred)));
#endif

        _lock_start = clock_t::now();
    }

    void unlock() {
        _lock_pred += ((clock_t::now() - _lock_start).count() - _lock_pred) / 8;

        _lock.clear(std::memory_order_release);
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#endif // MUTEXPP_HPP__

/******************************************************************************/

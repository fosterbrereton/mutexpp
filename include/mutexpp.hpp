/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef MUTEXPP_HPP__
#define MUTEXPP_HPP__

/******************************************************************************/

#include <chrono>
#include <atomic>

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

using clock_t = std::chrono::high_resolution_clock;
using tp_t = clock_t::time_point;
using tp_diff_t = decltype((std::declval<tp_t>() - std::declval<tp_t>()).count());
#if MUTEXPP_ENABLE_PROBE
using probe_t = void (*)(bool did_block, tp_diff_t new_p, tp_diff_t new_b);
#endif
/******************************************************************************/

class hybrid_spin_mutex_t
{
private:
    std::atomic<tp_diff_t> _spin_pred{0};
    std::atomic_flag       _lock{ATOMIC_FLAG_INIT};

public:
#if MUTEXPP_ENABLE_PROBE
    probe_t _probe;
#endif

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);
    }

    void lock() {
#if MUTEXPP_ENABLE_PROBE
        bool did_block{false};
#endif

        tp_t      spin_start{clock_t::now()};
        tp_diff_t spin_meas{0};

        while (!try_lock()) {
            spin_meas = (clock_t::now() - spin_start).count();

            if (spin_meas < _spin_pred * 2)
                continue;

            std::this_thread::sleep_for(std::chrono::nanoseconds(0));
#if MUTEXPP_ENABLE_PROBE
            did_block = true;
#endif
        }

        _spin_pred += (spin_meas -_spin_pred) / 8;

#if MUTEXPP_ENABLE_PROBE
        if (_probe)
            _probe(did_block, _spin_pred, 0);
#endif
    }

    void unlock() {
        _lock.clear(std::memory_order_release);
    }
};

/******************************************************************************/

class averse_hybrid_mutex_t
{
private:
    std::atomic<tp_diff_t> _spin_pred{0};
    std::atomic<tp_diff_t> _block_pred{0};
    std::atomic_flag       _lock{ATOMIC_FLAG_INIT};
    tp_t                   _lock_start;
    bool                   _did_block;

public:
#if MUTEXPP_ENABLE_PROBE
    probe_t _probe;
#endif

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);
    }

    void lock() {
        bool      did_block{false};
        tp_t      spin_start{clock_t::now()};
        tp_diff_t spin_meas{0};

        while (!try_lock()) {
            spin_meas = (clock_t::now() - spin_start).count();

            if (spin_meas < _spin_pred * 2)
                continue;

            std::this_thread::sleep_for(std::chrono::nanoseconds(0));

            did_block = true;
        }

        _did_block = did_block;
        _lock_start = clock_t::now();
        _spin_pred += (spin_meas -_spin_pred) / 8;

#if MUTEXPP_ENABLE_PROBE
        if (_probe)
            _probe(did_block, _spin_pred, _block_pred);
#endif
    }

    void unlock() {
        _lock.clear(std::memory_order_release);

        tp_diff_t lock_meas{(clock_t::now() - _lock_start).count()};

        _block_pred += (lock_meas - _block_pred) / 8;
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#endif // MUTEXPP_HPP__

/******************************************************************************/

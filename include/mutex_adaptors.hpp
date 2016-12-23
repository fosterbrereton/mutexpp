/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef MUTEX_ADAPTORS_HPP__
#define MUTEX_ADAPTORS_HPP__

/******************************************************************************/

#include <chrono>

/******************************************************************************/

namespace muads {

/******************************************************************************/

using clock_t = std::chrono::high_resolution_clock;
using tp_t = clock_t::time_point;
using tp_diff_t = decltype((std::declval<tp_t>() - std::declval<tp_t>()).count());
#if qDebug
using probe_t = void (*)(bool did_block, tp_diff_t new_p, tp_diff_t new_b);
#endif
/******************************************************************************/

template <typename Lockable>
class hybrid_adaptor
{
private:
    std::atomic<tp_diff_t> _spin_pred{0};
    Lockable               _mutex;

public:
#if qDebug
    probe_t _probe;
#endif

    void lock() {
#if qDebug
        bool      did_block{false};
#endif

        tp_t      spin_start = clock_t::now();
        tp_diff_t spin_meas{0};

        while (!_mutex.try_lock()) {
            spin_meas = (clock_t::now() - spin_start).count();

            if (spin_meas < _spin_pred * 2)
                continue;

            _mutex.lock();
#if qDebug
            did_block = true;
#endif

            break;
        }

        _spin_pred += (spin_meas -_spin_pred) / 8;

#if qDebug
        if (_probe)
            _probe(did_block, _spin_pred, 0);
#endif
    }

    void unlock() {
        _mutex.unlock();
    }
};

/******************************************************************************/

template <typename Lockable>
class avert_hybrid_adaptor
{
private:
    std::atomic<tp_diff_t> _spin_pred{0};
    std::atomic<tp_diff_t> _block_pred{0};
    Lockable               _m;

public:
#if qDebug
    probe_t _probe;
#endif

    void lock() {
#if qDebug
        bool did_block{false};
#endif
        tp_t      spin_start = clock_t::now();
        tp_diff_t spin_meas{0};

        if (_spin_pred > _block_pred) {
                tp_t block_start = clock_t::now();
                _m.lock();
                tp_diff_t block_meas = (clock_t::now() - block_start).count();
                _block_pred += (block_meas -_block_pred) / 8;
#if qDebug
                did_block = true;
#endif
        } else {
            while (!_m.try_lock()) {
                spin_meas = (clock_t::now() - spin_start).count();

                if (spin_meas < _spin_pred * 2)
                    continue;

                tp_t block_start = clock_t::now();
                _m.lock();
                tp_diff_t block_meas = (clock_t::now() - block_start).count();
                _block_pred += (block_meas -_block_pred) / 8;
#if qDebug
                did_block = true;
#endif

                break;
            }

            _spin_pred += (spin_meas -_spin_pred) / 8;
    }


#if qDebug
        if (_probe)
            _probe(did_block, _spin_pred, _block_pred);
#endif
    }

    void unlock() {
        _m.unlock();
    }
};

/******************************************************************************/

} // namespace muads

/******************************************************************************/

#endif // MUTEX_ADAPTORS_HPP__

/******************************************************************************/

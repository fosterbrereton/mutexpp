/******************************************************************************/
// Spin adaptor by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef SPIN_ADAPTOR_HPP__
#define SPIN_ADAPTOR_HPP__

/******************************************************************************/

#include <chrono>

/******************************************************************************/

template <typename Lockable>
class spin_adaptor
{
    using clock_t = std::chrono::high_resolution_clock;
    using time_point_t = clock_t::time_point;

public:
    using rep_t = decltype((std::declval<time_point_t>() - std::declval<time_point_t>()).count());

private:
    std::atomic<rep_t> predict_m{1};
    Lockable           mutex_m;

public:

#if qDebug
    void(*probe_m)(bool did_block, rep_t new_expected);
#endif

    void lock() {
        time_point_t  before{clock_t::now()};
        rep_t         measured{0};
#if qDebug
        bool          did_block{false};
#endif

        while (!mutex_m.try_lock()) {
            measured = (clock_t::now() - before).count();

            // we use predict_m each time through the loop because it might
            // be getting adjusted by other threads, and we want to take
            // advantage of their work.
            if (measured >= predict_m * 2) {
                mutex_m.lock();

#if qDebug
                did_block = true;
#endif
                break;
            }
        }

        // The larger the divisor, the less effect the current measured
        // count will have on the expected value. So, assuming a perfectly
        // stable area of contention, 4 would stabilize faster than 8.
        predict_m += (measured - predict_m) / 8;

#if qDebug
        if (probe_m)
            probe_m(did_block, predict_m);
#endif
    }

    void unlock() {
        mutex_m.unlock();
    }
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

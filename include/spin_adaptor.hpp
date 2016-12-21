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
    using rep_t = decltype((std::declval<time_point_t>() - std::declval<time_point_t>()).count());

    std::atomic<rep_t> expected_m{1};
    Lockable           mutex_m;

public:
    void lock() {
        thread_local time_point_t  before = clock_t::now();
        thread_local rep_t         elapsed{0};

        while (!mutex_m.try_lock()) {
            elapsed = (clock_t::now() - before).count();

            if (elapsed > (expected_m * 2)) {
                mutex_m.lock();

                break;
            }
        }

        expected_m += (elapsed - expected_m) / 8;
    }

    void unlock() {
        mutex_m.unlock();
    }
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

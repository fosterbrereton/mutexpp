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
    typedef std::chrono::high_resolution_clock clock_t;
    typedef clock_t::time_point                time_point_t;

    using rep_t = decltype((std::declval<time_point_t>() - std::declval<time_point_t>()).count());

    std::atomic<rep_t> expected_m{0};
    Lockable           mutex_m;

public:
    void lock() {
        thread_local time_point_t  before = clock_t::now();
        thread_local rep_t         elapsed{0};

        while (!mutex_m.try_lock()) {
            elapsed = (clock_t::now() - before).count();

            if (elapsed > expected_m) {
                mutex_m.lock();

                break;
            }
        }

        expected_m = (elapsed + 7 * expected_m) / 8;
    }

    void unlock() {
        mutex_m.unlock();
    }
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

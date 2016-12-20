/******************************************************************************/
// Spin adaptor by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef SPIN_ADAPTOR_HPP__
#define SPIN_ADAPTOR_HPP__

/******************************************************************************/

template <typename BasicLockable>
struct spin_adaptor
{
    void lock() {
        thread_local std::size_t count{0};
        thread_local bool        spinout{false};

        while (spin_m.exchange(true)) {
            if (++count >= expected_m) {
                spinout = true;

                mutex_m.lock();
            }
        }

        if (!spinout)
            mutex_m.lock();

        // compute new expected
        expected_m = (count + 7 * expected_m) / 8;
    }

    void unlock() {
        mutex_m.unlock();

        spin_m = false;
    }

private:
    std::atomic<bool>        spin_m{false};
    bool                     spin_out_m{false};
    std::atomic<std::size_t> expected_m{0};
    BasicLockable            mutex_m;
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

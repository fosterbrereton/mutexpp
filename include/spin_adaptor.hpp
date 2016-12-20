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
        mutex_m.lock();
    }

    void unlock() {
        mutex_m.unlock();        
    }

private:
    std::atomic<std::size_t> count_m;
    std::atomic<std::size_t> expected_m;
    BasicLockable            mutex_m;
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

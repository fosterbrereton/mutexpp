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

struct predictor_t {
    using clock_t = std::chrono::high_resolution_clock;
    using tp_t = clock_t::time_point;
    using rep_t = decltype((std::declval<tp_t>() - std::declval<tp_t>()).count());

    void start() {
        _s = clock_t::now();
    }

    bool within(double c = 2) {
        _sp = (clock_t::now() - _s).count();

        return _sp < _p * c;
    }

    void adjust() {
        _p += (_sp - _p) / 8;
    }

    rep_t get() const { return _p; }

private:
    tp_t               _s;
    rep_t              _sp;
    std::atomic<rep_t> _p{1};
};

/******************************************************************************/

template <typename Lockable>
class hybrid_adaptor
{
private:
    predictor_t _p;
    Lockable    _m;

public:

#if qDebug
    void(*_probe)(bool did_block, predictor_t::rep_t new_p);
#endif

    void lock() {
#if qDebug
        bool did_block{false};
#endif

        _p.start();

        while (!_m.try_lock()) {
            if (_p.within())
                continue;

            _m.lock();
#if qDebug
            did_block = true;
#endif

            break;
        }

        _p.adjust();

#if qDebug
        if (_probe)
            _probe(did_block, _p.get());
#endif
    }

    void unlock() {
        _m.unlock();
    }
};

/******************************************************************************/

#endif // SPIN_ADAPTOR_HPP__

/******************************************************************************/

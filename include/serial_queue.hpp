/******************************************************************************/
// Serial Queue by Foster Brereton, Sean Parent, and Felix Petriconi
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef MUTEXPP_SERIAL_QUEUE_HPP__
#define MUTEXPP_SERIAL_QUEUE_HPP__

/******************************************************************************/

// stdc++
#include <future>

/******************************************************************************/

#define MUTEXPP_SYSTEM_PORTABLE    0
#define MUTEXPP_SYSTEM_LIBDISPATCH 1
#define MUTEXPP_SYSTEM_WINDOWS     2

#if __APPLE__
    #ifndef MUTEXPP_SYSTEM
        #define MUTEXPP_SYSTEM MUTEXPP_SYSTEM_LIBDISPATCH
    #endif
#elif _MSC_VER
    #ifndef MUTEXPP_SYSTEM
        #define MUTEXPP_SYSTEM MUTEXPP_SYSTEM_WINDOWS
    #endif
#endif

#ifndef MUTEXPP_SYSTEM
    #define MUTEXPP_SYSTEM MUTEXPP_SYSTEM_PORTABLE
#endif

/******************************************************************************/

#if (MUTEXPP_SYSTEM == MUTEXPP_SYSTEM_LIBDISPATCH)
    #include <dispatch/dispatch.h>
#endif

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

namespace detail {

template <typename T>
using result_of_t = typename std::result_of<T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <class Function, class... Args>
using result_type = result_of_t<decay_t<Function>(decay_t<Args>...)>;

} // namespace detail

/******************************************************************************/

class serial_queue_t {
    dispatch_queue_t _q = dispatch_queue_create("com.mutexpp.serial_queue", NULL);

  public:
    serial_queue_t() = default;
    serial_queue_t(const char* name) : _q{ dispatch_queue_create(name, NULL) } { }
    ~serial_queue_t() { dispatch_release(_q); }

    template <class Function, class... Args>
    auto async(Function&& f, Args&&... args) -> std::future<detail::result_type<Function, Args...>> {
        using result_type = detail::result_type<Function, Args...>;
        using packaged_type = std::packaged_task<result_type()>;

        auto p = new packaged_type(std::bind([f](Args&&... args) {
            return f(std::move(args)...);
        }, std::forward<Args>(args)...));

        auto result = p->get_future();

        dispatch_async_f(_q,
                p, [](void* f_) {
                    packaged_type* f = static_cast<packaged_type*>(f_);
                    (*f)();
                    delete f;
                });

        return result;
    }

    template <class Function, class... Args>
    auto sync(Function&& f, Args&&... args) -> detail::result_type<Function, Args...> {
        return async(std::forward<Function>(f), std::forward<Args>(args)...).get();
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#endif // MUTEXPP_SERIAL_QUEUE_HPP__

/******************************************************************************/

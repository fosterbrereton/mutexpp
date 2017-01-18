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

#define MUTEXPP_SERIAL_QUEUE_PORTABLE    0
#define MUTEXPP_SERIAL_QUEUE_LIBDISPATCH 1
#define MUTEXPP_SERIAL_QUEUE_WINMF       2

#ifndef MUTEXPP_SERIAL_QUEUE_IMPL
    #if __APPLE__
        #define MUTEXPP_SERIAL_QUEUE_IMPL MUTEXPP_SERIAL_QUEUE_LIBDISPATCH
    #elif _MSC_VER
        #define MUTEXPP_SERIAL_QUEUE_IMPL MUTEXPP_SERIAL_QUEUE_WINMF
    #else
        #define MUTEXPP_SERIAL_QUEUE_IMPL MUTEXPP_SERIAL_QUEUE_PORTABLE
    #endif
#endif

/******************************************************************************/

#if (MUTEXPP_SERIAL_QUEUE_IMPL == MUTEXPP_SERIAL_QUEUE_LIBDISPATCH)

/******************************************************************************/

#include <dispatch/dispatch.h>

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

namespace detail {

/******************************************************************************/

template <typename T>
using result_of_t = typename std::result_of<T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <class Function, class... Args>
using result_type = result_of_t<decay_t<Function>(decay_t<Args>...)>;

/******************************************************************************/

} // namespace detail

/******************************************************************************/

class serial_queue_t {
    dispatch_queue_t _q = dispatch_queue_create("com.mutexpp.serial_queue", NULL);

public:
    serial_queue_t() = default;

    serial_queue_t(const serial_queue_t& rhs) : _q(rhs._q) {
        dispatch_retain(_q);
    }

    serial_queue_t(const char* name) : _q{ dispatch_queue_create(name, NULL) } {
    }

    ~serial_queue_t() {
        dispatch_release(_q);
    }

    serial_queue_t& operator=(const serial_queue_t& rhs) {
        if (_q != rhs._q) {
            dispatch_release(_q);
            _q = rhs._q;
            dispatch_retain(_q);
        }

        return *this;
    }

    template <class Function, class... Args>
    std::future<detail::result_type<Function, Args...>> async(Function&& f, Args&&... args) {
        using result_type = detail::result_type<Function, Args...>;
        using packaged_type = std::packaged_task<result_type()>;

        auto p = new packaged_type(std::bind([f](Args&&... args) {
            return f(std::move(args)...);
        }, std::forward<Args>(args)...));

        auto result = p->get_future();

        dispatch_async_f(_q,
                         p,
                         [](void* f_) {
                             packaged_type* f = static_cast<packaged_type*>(f_);
                             (*f)();
                             delete f;
                         });

        return result;
    }

    template <class Function, class... Args>
    detail::result_type<Function, Args...> sync(Function&& f, Args&&... args) {
        return async(std::forward<Function>(f), std::forward<Args>(args)...).get();
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#elif (MUTEXPP_SERIAL_QUEUE_IMPL == MUTEXPP_SERIAL_QUEUE_WINMF)

/******************************************************************************/

#include <mfapi.h>
#include <shlwapi.h>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "shlwapi")

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

namespace detail {

/******************************************************************************/

template <class Function, class... Args>
using result_type = decltype(std::declval<Function>()(std::declval<Args>()...));

/******************************************************************************/

struct mf_init_t {
    mf_init_t() {
        if (MFStartup(MF_VERSION, MFSTARTUP_LITE) != S_OK)
            throw std::runtime_error("MFStartup failed");

        _ok = true;
    }
    ~mf_init_t() {
        if (_ok)
            MFShutdown();
    }
    bool _ok{false};
};

/******************************************************************************/

template <typename ResultType, typename PackagedType>
struct async_wrapper : public IMFAsyncCallback {
    PackagedType _p;
    LONG         _rc{1};

    template <typename F>
    explicit async_wrapper(F&& f) : _p(std::forward<F>(f)) { }

    std::future<ResultType> get_future() { return _p.get_future(); }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        #pragma warning (push)
        #pragma warning (disable:4838) // DWORD to int narrowing conversion
        static const QITAB qit[] = {
            QITABENT(async_wrapper, IMFAsyncCallback),
            { 0 }
        };
        #pragma warning (pop)
        return QISearch(this, qit, riid, ppv);
    }

    STDMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&_rc);
    }

    STDMETHODIMP_(ULONG) Release() {
        LONG cRef = InterlockedDecrement(&_rc);
        if (cRef == 0) {
            delete this;
        }
        return cRef;
    }

    STDMETHODIMP GetParameters(DWORD*, DWORD*) {
        return E_NOTIMPL;
    }

    STDMETHODIMP Invoke(IMFAsyncResult* result) {
        _p();
        delete this;
        return S_OK;
    }
};

/******************************************************************************/

} // namespace detail

/******************************************************************************/

class serial_queue_t {
    DWORD _q{0};

public:
    serial_queue_t() = default;

    serial_queue_t(const serial_queue_t& rhs) :
        _q(rhs._q) {
        MFLockWorkQueue(_q);
    }

    serial_queue_t(const char* name) {
        HRESULT hr = MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &_q);

        if (hr != S_OK)
            throw std::bad_alloc();
    }

    ~serial_queue_t() {
        MFUnlockWorkQueue(_q);
    }

    serial_queue_t& operator=(const serial_queue_t& rhs) {
        if (_q != rhs._q) {
            MFUnlockWorkQueue(_q);
            _q = rhs._q;
            MFLockWorkQueue(_q);
        }

        return *this;
    }

    template <class Function, class... Args>
    std::future<detail::result_type<Function, Args...>> async(Function&& f, Args&&... args) {
        static const detail::mf_init_t init_s;

        using result_type = detail::result_type<Function, Args...>;
        using packaged_type = std::packaged_task<result_type()>;

        auto p = new detail::async_wrapper<result_type, packaged_type>(std::bind([f](Args&&... args) {
            return f(std::move(args)...);
        }, std::forward<Args>(args)...));

        auto result = p->get_future();

        if (MFPutWorkItem(_q, p, nullptr) != S_OK)
            throw std::runtime_error("MFPutWorkItem failed");

        return result;
    }

    template <class Function, class... Args>
    detail::result_type<Function, Args...> sync(Function&& f, Args&&... args) {
        return async(std::forward<Function>(f), std::forward<Args>(args)...).get();
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#elif (MUTEXPP_SERIAL_QUEUE_IMPL == MUTEXPP_SERIAL_QUEUE_PORTABLE)

/******************************************************************************/

#include <condition_variable>
#include <deque>
#include <thread>

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

namespace detail {

/******************************************************************************/

template <class Function, class... Args>
using result_type = decltype(std::declval<Function>()(std::declval<Args>()...));

/******************************************************************************/

} // namespace detail

/******************************************************************************/

class serial_queue_t {
    typedef std::unique_lock<std::mutex> lock_t;

    typedef std::pair<void(*)(void*), void*> pair_t;

    std::mutex              _mutex;
    std::condition_variable _ready;
    std::deque<pair_t>      _queue;
    bool                    _done{false};
    std::thread             _executor;

    void run() {
        while (true) {
            lock_t lock(_mutex);

            _ready.wait(lock, [this](){ return !_queue.empty() || _done; });

            if (!_queue.empty()) {
                pair_t pair = std::move(_queue.front());

                _queue.pop_front();

                lock.unlock();

                pair.first(pair.second);
            } else if (_done) {
                break;
            }
        }
    }

    template <typename TaskType>
    static void invoke(void* p) {
        TaskType* f = static_cast<TaskType*>(p);
        (*f)();
        delete f;
    }

    template <typename TaskType>
    void dispatch(TaskType* task) {
        pair_t pair{&invoke<TaskType>, task};
        lock_t lock(_mutex);

        _queue.emplace_back(std::move(pair));

        lock.unlock();

        _ready.notify_one();
    }

public:
    serial_queue_t() :
        _executor(&serial_queue_t::run, this)
    { }

    ~serial_queue_t() {
        lock_t lock(_mutex);

        _done = true;
        lock.unlock();
        _ready.notify_one();
        _executor.join();
    }

    template <class Function, class... Args>
    std::future<detail::result_type<Function, Args...>> async(Function&& f, Args&&... args) {
        using result_type = detail::result_type<Function, Args...>;
        using packaged_type = std::packaged_task<result_type()>;

        auto p = new packaged_type(std::bind([f](Args&&... args) {
            return f(std::move(args)...);
        }, std::forward<Args>(args)...));

        auto result = p->get_future();

        dispatch(p);

        return result;
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#endif // MUTEXPP_SERIAL_QUEUE_IMPL

/******************************************************************************/

namespace mutexpp {

/******************************************************************************/

template <typename T>
class serial_wrapper {
    serial_queue_t _q;
    T              _r;

public:
    typedef T value_type;

    template <typename... Args>
    explicit serial_wrapper(Args&&... args) : _r(std::forward<Args>(args)...) {
    }

    template <typename F>
    auto operator()(F&& f) -> decltype(_q.async(std::bind(std::forward<F>(f), std::ref(_r)))) {
        return _q.async(std::bind(std::forward<F>(f), std::ref(_r)));
    }
};

/******************************************************************************/

} // namespace mutexpp

/******************************************************************************/

#endif // MUTEXPP_SERIAL_QUEUE_HPP__

/******************************************************************************/

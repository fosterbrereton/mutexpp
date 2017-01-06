`mutexpp` is a collection of C++ mutex implementations and utilities.

# Overview

## What is a blocking mutex?

A mutex that, upon locking, will halt a thread's execution until that mutex is acquired.

These types of mutexes are best used when the critical sections they protect are relatively long (such as I/O or OS API calls).

### Pros

When blocked, the thread consumes almost no computer resources.

### Cons

It is very slow to transition a blocking mutex from unlocked to locked.

### Examples

[`std::mutex`](http://en.cppreference.com/w/cpp/thread/mutex)

## What is a spin mutex?

A mutex that, upon locking, will loop a thread's execution until that mutex is acquired.

These types of mutexes are best used when the critical sections they protect are relatively brief (such as memory-resident data modifications).

### Pros

When blocked, the thread will spike a CPU.

### Cons

It is very fast to transition a spin mutex from unlocked to locked.

### Examples

[`tbb::spin_mutex`](https://software.intel.com/en-us/node/506269)

## What is a 'hybrid' mutex?

A mutex that, upon locking, will first spin, then halt a thread's execution until that mutex is acquired. The amount of time taken during the initial spin is adjusted such that the lock acquistion happens more often during the spin phase.

These mutexes are best used when spin mutexes would suffice, but occasionally a critical section may take an unusually long amount of time, and it would be better to block the thread.

### Pros

Tries to maximize throughput while minimizing computer resource consumption. The spin phase is an attempt to avoid the expensive cost of blocking the thread, while the blocking phase attempts to avoid the expense of spiking a CPU in the event locking is taking too long.

### Cons

If the time required to lock the mutex is consistently large (i.e., beyond the time it takes to block/unblock a thread) you'll be spending time spinning when you're better off using a blocking mutex.

# Notes

Process scheduling is not considered.

## Adaptive computation derivation

After every mutex lock, we adjust the expected spin time. Given $p_n$ to be the predictive spin time and $m$ to be the measured spin time, 

$$
\begin{align}
  p_{n+1} &= \frac{7}{8} p_n + \frac{1}{8} m \\
8 p_{n+1} &= 7 p_n + m \\
8 p_{n+1} &= 8 p_n + m - p_n \\
  p_{n+1} &= p_n + \frac{m - p_n}{8} \\
\end{align}
$$

# See Also

## Mutex related

- A [description](http://stackoverflow.com/a/25168942/153535) of `pthread`'s adaptive mutex from the original implementer, Kaz Kylheku
- Intel TBB description of [mutex flavors](https://www.threadingbuildingblocks.org/docs/help/tbb_userguide/Mutex_Flavors.html).

## Clock & Timing related

- POSIX API [`clock_gettime`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html)
- Wall v. CPU clock [example implementation](http://en.cppreference.com/w/cpp/chrono/c/clock)
- [Example clock calls](http://nadeausoftware.com/articles/2012/04/c_c_tip_how_measure_elapsed_real_time_benchmarking) on various OSes.
- [`mach_absolute_time`](https://developer.apple.com/library/content/qa/qa1398/_index.html)

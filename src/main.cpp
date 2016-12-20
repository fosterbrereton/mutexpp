// stdc++
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// spin_adaptor
#include "spin_adaptor.hpp"

typedef spin_adaptor<std::mutex> spin_t;
typedef std::unique_lock<spin_t> spinlock_t;

int main(int argc, char** argv) {
    std::vector<std::thread> pool;
    spin_t                   mutex;

    for (std::size_t i(0); i < 5; ++i) {
        pool.emplace_back([&](){
            spinlock_t lock(mutex);
            std::cout << "Hello, world!\n";
        });
    }

    for (auto& t : pool) {
        t.join();
    }
}

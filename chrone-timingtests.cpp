// C++ program to illustrate time point
// and system clock functions
#include <chrono> //NOLINT
#include <iostream>
#include "chrone.hpp"
int64_t fibonacci(unsigned n) {
    if (n < 2) return n;
    return fibonacci(n - 1) + fibonacci(n-2);
}

int main() {
    chrone firstChrone("firstChrone.csv");
    {
        int iterations = 100;
        volatile int64_t result = 0;
        timer globaltimer("Fibonacci34_meanoveriterations", &firstChrone, iterations);
        for (int i = 0; i < iterations; ++i){
            result = fibonacci(34);
        }
    }

    chrone secondChrone("secondChrone.csv");
    {
        int iterations = 100;
        volatile int64_t result = 0;
        for (int i = 0; i < iterations; ++i){
            timer local_timer("Fibonacci34each", &secondChrone);
            result = fibonacci(34);
        }
    }
}

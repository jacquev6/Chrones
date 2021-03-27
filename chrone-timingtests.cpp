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
    chrone mainChrone("main_test.csv");
    {
        int iterations = 100;
        volatile int64_t result = 0;
        timer local_timer("Fibonacci34mean", &mainChrone, iterations);
        for (int i = 0; i < iterations; ++i){
            result = fibonacci(34);
        }
    }

    chrone secondChrone("second_test.csv");
    {
        int iterations = 100;
        volatile int64_t result = 0;
        for (int i = 0; i < iterations; ++i){
            timer local_timer("Fibonacci34each", &secondChrone);
            result = fibonacci(34);
        }
    }
}

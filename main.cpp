// C++ program to illustrate time point
// and system clock functions
#include <chrono> //NOLINT
#include <iostream>
#include "chrone.hpp"
// Function to calculate
// Fibonacci series
int64_t fibonacci(unsigned n) {
    if (n < 2) return n;
    return fibonacci(n - 1) + fibonacci(n-2);
}

int main() {
    chrone mainChrone("main_monitor");
    for (int i = 3; i < 42; ++i) {
        timer local_timer("Fibonacci", &mainChrone);
        fibonacci(i);
    }
}

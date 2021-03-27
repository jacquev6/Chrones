// C++ program to illustrate the different usages
// of the class chrone
#include <chrono> //NOLINT
#include <iostream>
#include "chrone.hpp"

int64_t fibonacci(unsigned n) {
    if (n < 2) return n;
    return fibonacci(n - 1) + fibonacci(n-2);
}

int main() {
    chrone mainChrone("main_monitor.csv");
    // The chrone is created and can now hosts many timers
    
    for (int i = 3; i < 42; ++i) {
        timer local_timer("Fibonacci", &mainChrone);
        // Here the timer `local_timer` is created, initialized 
        // with its creation timestamp and add to mainChrone
        // that will export the result in `main_monitor.csv`
        // when mainChrone is destroy (end of scope). 
        fibonacci(i);
    } // each time `local_timer` is out of scope, its life duration is computed and saved in `chrone`.
}

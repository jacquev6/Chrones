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
        timer local_timer("Fibonacci-Each", &mainChrone);
        // Here the timer `local_timer` is created, initialized
        // with its creation timestamp and add to mainChrone
        // that will export the result in `main_monitor.csv`
        // when mainChrone is destroyed (end of scope).
        fibonacci(i);
    }  // each time `local_timer` is out of scope, its life duration is computed and saved in `main_monitor.csv`.

    {
        const unsigned int nb_iterations = 10;
        timer global_timer("Fibonacci-Mean", &mainChrone, nb_iterations);
        // Here the timer `global_timer` is created, initialized
        // with its creation timestamp and add to mainChrone
        // the result will be divided by `nb_iterations` when
        // the timer is destroyed
        for (int i = 1; i < nb_iterations; ++i) {
            fibonacci(40);
        }
    }  // each time `global_timer` is out of scope, its life duration
       // is computed, divided by `nb_iterations` and saved in `main_monitor.csv`.
}

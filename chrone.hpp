#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>
#include <iostream>

class timer {
    public:


        timer();
        ~timer();
        timer(std::string label);
        std::string getLabel();
        

    private:
        std::string _label;
        std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> _stop_time;
        long int _elapsed_time;
};

#endif
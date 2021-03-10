#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>
#include <iostream>
#include <vector>

using clk = std::chrono::high_resolution_clock;

class chrone {
    public:
        chrone();
        ~chrone();
        void appendTimer(std::string label, long int _elapsed_time);
        long int getTimeOfTimer(unsigned int timer_index);
        std::string getLabelOfTimer(unsigned int timer_index);
        long int getSize();
        
    private:
        std::vector<std::string> _stable_label;
        std::vector<long int> _stable_time;
        
};

class timer {
    public:
        timer(std::string label, chrone *handle);        
        ~timer();

    private:
        std::string _label;
        chrone *_handle;
        std::chrono::time_point<clk> _start_time;
        std::chrono::time_point<clk> _stop_time;
        volatile long int _elapsed_time;
};


#endif
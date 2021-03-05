#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>
#include <iostream>
#include <vector>

class chrone {
    public:
        chrone();
        ~chrone();
        void appendTimer(std::string label, long int _elapsed_time);
    private:
        std::vector<std::string> _stable_label;
        std::vector<long int> _stable_time;
        
};

class timer {
    public:


        timer();
        ~timer();
        timer(std::string label, chrone *handle);
        std::string getLabel();
        

    private:
        std::string _label;
        chrone *_handle;
        std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> _stop_time;
        long int _elapsed_time;
};


#endif
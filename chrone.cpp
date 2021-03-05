#include "chrone.hpp"

timer::timer() {}

timer::timer(std::string label)
{
    _label = label;
    _start_time = std::chrono::high_resolution_clock::now();
}

timer::~timer() 
{
    _stop_time = std::chrono::high_resolution_clock::now();
    _elapsed_time = (_stop_time - _start_time).count();
    std::cout << _label << ": " << _elapsed_time << "ns\n";
}

std::string timer::getLabel()
{
    return _label;
}

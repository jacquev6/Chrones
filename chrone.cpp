#include "chrone.hpp"

timer::timer() {}

timer::timer(std::string label, chrone *handle)
{
    _label = label;
    _handle = handle;
    _start_time = clk::now();
}

timer::~timer() 
{
    _stop_time = clk::now();
    _elapsed_time = (_stop_time - _start_time).count();
    _handle->appendTimer(_label, _elapsed_time);

}

chrone::chrone() {}

chrone::~chrone() {
    for(int i = 0; i!= _stable_label.size(); ++i){
        std::cout << _stable_label[i] << ": " << _stable_time[i] << "ns\n";
    }
}

void chrone::appendTimer(std::string label, long int elapsed_time)
 {
   _stable_label.push_back(label);
   _stable_time.push_back(elapsed_time);
 }

long int chrone::getTimeOfTimer(unsigned int timer_index)
{
   return _stable_time[timer_index];
}
 
std::string chrone::getLabelOfTimer(unsigned int timer_index)
{
   return _stable_label[timer_index];
}

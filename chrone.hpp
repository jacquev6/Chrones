#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>  //NOLINT
#include <iostream>
#include <vector>
#include <string>

using clk = std::chrono::high_resolution_clock;

class chrone {
 public:
  chrone();
  ~chrone();
  void appendTimer(std::string label, int64_t _elapsed_time);
  int64_t getTimeOfTimer(unsigned int timer_index);
  std::string getLabelOfTimer(unsigned int timer_index);
  int64_t getSize();

 private:
  std::vector<std::string> _stable_label;
  std::vector<int64_t> _stable_time;
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
  volatile int64_t _elapsed_time;
};
#endif  // CHRONE_HPP_

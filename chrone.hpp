#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>  //NOLINT
#include <vector>
#include <string>
#include <utility>


class chrone {
 public:
  explicit chrone(std::string filename);
  ~chrone();
  int64_t getSize();
  int64_t getDuration(const std::string& label);
  void appendTimer(std::string label, int64_t _elapsed_time);

 private:
  std::vector<std::pair<std::string, int64_t>> _rack;
  std::string _filename;
};

template<typename clk>
class timer_ {
 public:
  timer_(std::string label, chrone *handle, int64_t nb_of_iterations = 1);
  ~timer_();

 private:
  std::string _label;
  chrone *_handle;
  std::chrono::time_point<clk> _start_time;
  std::chrono::time_point<clk> _stop_time;
  int64_t _elapsed_time;
  int _nb_of_iterations;
};

template<typename clk>
timer_<clk>::timer_(std::string label, chrone *handle, int64_t nb_of_iterations) {
    _label = label;
    _handle = handle;
    _nb_of_iterations = nb_of_iterations;
    _start_time = clk::now();
}

template<typename clk>
timer_<clk>::~timer_()  {
    std::chrono::time_point<clk> stop_time = clk::now();
    int64_t elapsed_time = ((stop_time - _start_time).count())/_nb_of_iterations;
    _handle->appendTimer(_label, elapsed_time);
}

typedef timer_<std::chrono::high_resolution_clock> timer;

#endif  // CHRONE_HPP_

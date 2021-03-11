#ifndef CHRONE_HPP_
#define CHRONE_HPP_

#include <chrono>  //NOLINT
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <utility>


using clk = std::chrono::high_resolution_clock;

class chrone {
 public:
  explicit chrone(std::string filename);
  ~chrone();
  void appendTimer(std::string label, int64_t _elapsed_time);
  int64_t getSize();

 private:
  std::vector<std::pair<std::string, int64_t>> _rack;
  std::string _filename;
};

class timer {
 public:
  timer(std::string label, chrone *handle, int64_t nb_of_iterations=1);
  ~timer();

 private:
  std::string _label;
  chrone *_handle;
  std::chrono::time_point<clk> _start_time;
  int _nb_of_iterations;
};
#endif  // CHRONE_HPP_

// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#ifndef CHRONES_HPP_
#define CHRONES_HPP_

#ifdef NO_CHRONES

#define CHRONABLE(name)
#define CHRONE(...) do {} while (false)

#else

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>  // NOLINT(build/c++11)
#include <chrono>  // NOLINT(build/c++11)
#include <atomic>

#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include <boost/lockfree/queue.hpp>


// The Chrones library instruments your code to measure the time taken by each function.

// Usage:
// In your main file, use the CHRONABLE macro, giving it the base name of the
// csv file you want:
//     CHRONABLE("my-exec")
// The code above will generate `my-exec.chrones.csv` in the working directory.
// You can then use `chrones.py report my-exec.chrones.csv` to generate a report.

// Then in the functions you want to instrument, use the CHRONE macro.
// This macro accepts optional `label` and `index` parameters, in that order.
// They can be omitted when instrumenting a whole function:
//     void f() {
//       CHRONE();
//       // body
//     }
//
// The `label` is useful when there are several logical blocks in the function:
//     void f() {
//       CHRONE();
//       {
//         CHRONE("block A");
//         // block
//       }
//       {
//         CHRONE("block B");
//         // block
//       }
//     }
// Note that each block must be in its own set of curly braces.
//
// The `index` is useful to measure several iterations of a loop:
//     void f() {
//       CHRONE();
//       for (int i = 0; i != 16; ++i) {
//         CHRONE("loop", i);
//         // body
//       }
//     }
// Note that you must add a `label` to be able to give an `index`.

// To deactivate Chrones in a given file, just `#define NO_CHRONES` before `#include <chrones.hpp>`.
// (You can also call `g++` with `-DNO_CHRONES` to deactivate Chrones in the whole project)

// Known current limitations of the Chrones libraries:
// - it uses at least one GCC extension (__VA_OPT__)
// - it is not safe to use outside main (i.e. during initialization of global and static variables)
// - it is not tested on recursive code. It might or might not work in that case.
// These limitation might be removed in future versions of the library.

// Troubleshooting:
// - "undefined reference to chrones::global_coordinator": double-check you called CHRONABLE
// - "undefined reference to chrones::<<anything else>>": double-check you linked with chrones.o

namespace chrones {

std::string quote_for_csv(std::string);

struct event {
  int process_id;
  std::size_t thread_id;
  int64_t time;
  std::vector<std::string> attributes;
};

class coordinator_base {
 public:
  explicit coordinator_base(std::ostream& stream);
  ~coordinator_base();

  void add_event(const event& event);

 private:
  void work();

 private:
  boost::lockfree::queue<const event*> _events;
  std::ostream& _stream;
  std::atomic_bool _done;
  // Using boost::thread instead of std::thread to avoid this segmentation fault:
  // https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
  boost::thread _worker;  // Keep _worker last: all other members must be fully constructed before it starts
};

template<typename Info>
class coordinator_tmpl : public coordinator_base {
 public:
  using coordinator_base::coordinator_base;

  // @todo Add logs of level TRACE
  void start_stopwatch(
    const std::string& function,
    const boost::optional<std::string>& label,
    const boost::optional<int> index
  ) {
    add_event(event {
      Info::get_process_id(),
      Info::get_thread_id(),
      Info::get_time(),
      {
        "sw_start",
        function,
        label == boost::none ? "-" : quote_for_csv(*label),
        index == boost::none ? "-" : std::to_string(*index),
      }
    });
  }

  void stop_stopwatch() {
    add_event(event {
      Info::get_process_id(),
      Info::get_thread_id(),
      Info::get_time(),
      {
        "sw_stop",
      }
    });
  }
};

template<typename Info>
class stopwatch_tmpl {
 public:
  stopwatch_tmpl(
    coordinator_tmpl<Info>* coordinator,
    const std::string& function,
    const std::string& label,
    const boost::optional<int> index = boost::none) :
      stopwatch_tmpl(coordinator, function, boost::optional<std::string>(label), index)
  {};

  stopwatch_tmpl(
    coordinator_tmpl<Info>* coordinator,
    const std::string& function,
    const boost::optional<std::string>& label = boost::none,
    const boost::optional<int> index = boost::none) : _coordinator(coordinator) {
    coordinator->start_stopwatch(function, label, index);
  }

  ~stopwatch_tmpl() {
    _coordinator->stop_stopwatch();
  }

 private:
  stopwatch_tmpl(const stopwatch_tmpl&) = delete;
  stopwatch_tmpl(const stopwatch_tmpl&&) = delete;
  stopwatch_tmpl& operator=(const stopwatch_tmpl&) = delete;

  coordinator_tmpl<Info>* _coordinator;
};

struct RealInfo {
  static std::chrono::steady_clock::time_point startup_time;

  static int64_t get_time() {
    return std::chrono::nanoseconds(std::chrono::steady_clock::now() - startup_time).count();
  }

  static int get_process_id() {
    return ::getpid();
  }

  static std::size_t get_thread_id() {
    return std::hash<std::thread::id>()(std::this_thread::get_id());
  }
};

typedef stopwatch_tmpl<RealInfo> stopwatch;
typedef coordinator_tmpl<RealInfo> coordinator;

extern coordinator global_coordinator;

}  // namespace chrones

#define CHRONABLE(name) \
  namespace chrones { \
    std::ofstream global_stream(name + std::string(".chrones.csv"), std::ios_base::app); \
    coordinator global_coordinator(global_stream); \
  }

#ifdef __CUDA_ARCH__
#define CHRONE(...)
#else
// Variadic macro that forwards its arguments to the appropriate chrones::stopwatch constructor
#define CHRONE(...) chrones::stopwatch chrone_stopwatch##__line__( \
  &chrones::global_coordinator, chrones::quote_for_csv(__PRETTY_FUNCTION__) \
  __VA_OPT__(,) __VA_ARGS__)  // NOLINT(whitespace/comma)
#endif

#endif  // NO_CHRONES

#endif  // CHRONES_HPP_
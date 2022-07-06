// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#ifndef CHRONES_HPP_
#define CHRONES_HPP_

#ifdef NO_CHRONES

#define CHRONABLE(name)
#define CHRONE(...) do {} while (false)

#else

#include <atomic>
#include <chrono>  // NOLINT(build/c++11)
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <tuple>
#include <vector>

#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include <boost/lockfree/queue.hpp>

#include "stream-statistics.hpp"


// The Chrones library instruments your code to measure the time taken by each function.

// Usage:
// In your main file, use the CHRONABLE macro, giving it the base name of the
// csv file you want:
//     CHRONABLE("my-exec")
// The code above will generate `my-exec.[PID].chrones.csv` in the working directory.
// You can then use `chrones.py report my-exec.[PID].chrones.csv` to generate a report.
// You can also `cat my-exec.*.chrones.csv > my-exec.chrones.csv` and then
// `chrones.py report my-exec.chrones.csv` to generate a report about several
// executions of `my-exec`.

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

template<typename Info>
class coordinator_tmpl {
 public:
  explicit coordinator_tmpl(std::ostream& stream) :
    _events(1024),  // Arbitrary initial capacity that can grow anyway
    _stream(stream),
    _done(false),
    _worker(&coordinator_tmpl<Info>::work, this) {}

  ~coordinator_tmpl() {
    add_summary_events();
    _done = true;
    _worker.join();
  }

 public:
  // @todo Add logs of level TRACE
  int64_t start_stopwatch(
    const std::string& function,
    const boost::optional<std::string>& label,
    const boost::optional<int> index
  ) {
    const int64_t start_time = Info::get_time();
    add_event(event {
      Info::get_process_id(),
      Info::get_thread_id(),
      start_time,
      {
        "sw_start",
        function,
        label == boost::none ? "-" : quote_for_csv(*label),
        index == boost::none ? "-" : std::to_string(*index),
      }
    });
    return start_time;
  }

  void stop_stopwatch(
    const std::string& function,
    const boost::optional<std::string>& label,
    int64_t start_time
  ) {
    const int64_t stop_time = Info::get_time();
    const int process_id = Info::get_process_id();
    const std::size_t thread_id = Info::get_thread_id();
    add_event({ process_id, thread_id, stop_time, { "sw_stop" }});
    accumulate(process_id, thread_id, function, label, stop_time - start_time);
  }

 private:
  void add_summary_events() {
    const int64_t stop_time = Info::get_time();
    boost::lock_guard<boost::mutex> guard(_statistics_mutex);
    for (const auto& stat : _statistics) {
      int process_id;
      std::size_t thread_id;
      std::string function;
      boost::optional<std::string> label;
      std::tie(process_id, thread_id, function, label) = stat.first;
      add_event({
        process_id,
        thread_id,
        stop_time,
        {
          "sw_summary",
          function,
          label == boost::none ? "-" : quote_for_csv(*label),
          std::to_string(stat.second.count()),
          std::to_string(static_cast<int64_t>(stat.second.mean())),
          std::to_string(static_cast<int64_t>(stat.second.standard_deviation())),
          std::to_string(static_cast<int64_t>(stat.second.min())),
          std::to_string(static_cast<int64_t>(stat.second.median())),
          std::to_string(static_cast<int64_t>(stat.second.max())),
          std::to_string(static_cast<int64_t>(stat.second.sum())),
        }});
    }
  }

  void add_event(const event& e) {
    _events.push(new event(e));
  }

  void accumulate(
      int process_id,
      std::size_t thread_id,
      const std::string& function,
      const boost::optional<std::string>& label,
      const int64_t duration) {
    boost::lock_guard<boost::mutex> guard(_statistics_mutex);
    _statistics[std::make_tuple(process_id, thread_id, function, label)].update(duration);
  }

  void work() {
    // There is no blocking `pop` on a boost::lockfree::queue because it would defeat its very purpose.
    // But we only care about `push` being lock-free, so we emulate a blocking `pop` with
    // this "poll, sleep" loop.
    while (true) {
      _events.consume_all([this](const event* event) {
        _stream << event->process_id << ',' << event->thread_id << ',' << event->time;
        for (auto& attribute : event->attributes) {
          _stream << ',' << attribute;
        }
        _stream  << '\n';  // No std::endl: don't flush each line, improve performance
        delete event;
      });

      if (_done) {
        break;
      } else {
        // Avoid using 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

 private:
  boost::lockfree::queue<const event*> _events;
  std::ostream& _stream;
  std::atomic_bool _done;
  std::map<
    std::tuple<int, std::size_t, std::string, boost::optional<std::string>>,
    StreamStatistics
  > _statistics;
  // Using boost::thread instead of std::thread to avoid this segmentation fault:
  // https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
  boost::mutex _statistics_mutex;
  boost::thread _worker;  // Keep _worker last: all other members must be fully constructed before it starts
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
      const boost::optional<int> index = boost::none) :
    _coordinator(coordinator),
    _function(function),
    _label(label),
    _start_time(coordinator->start_stopwatch(function, label, index))
  {}

  ~stopwatch_tmpl() {
    _coordinator->stop_stopwatch(_function, _label, _start_time);
  }

 private:
  stopwatch_tmpl(const stopwatch_tmpl&) = delete;
  stopwatch_tmpl(const stopwatch_tmpl&&) = delete;
  stopwatch_tmpl& operator=(const stopwatch_tmpl&) = delete;

  coordinator_tmpl<Info>* _coordinator;
  const std::string _function;
  const boost::optional<std::string> _label;
  int64_t _start_time;
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
    std::ofstream global_stream( \
      std::string(name) + "." + std::to_string(::getpid()) + ".chrones.csv", std::ios_base::app); \
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

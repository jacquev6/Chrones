// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#ifndef CHRONES_HPP_
#define CHRONES_HPP_

#ifdef NO_CHRONES

#define CHRONABLE(name)

#define CHRONE(...) do {} while (false)

#define MINICHRONE(...) do {} while (false)

#else

#include <algorithm>
#include <atomic>
#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <tuple>
#include <utility>
#include <vector>

#include <boost/optional.hpp>
#include <boost/thread.hpp>


// The Chrones library instruments your code to measure the time taken by code blocks.

// Usage:
// In your main file, use the CHRONABLE macro, giving it the base name of the csv file you want:
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
// - it is not safe to use outside main (e.g. during initialization of global and static variables)
// - it is not tested on recursive code. It might or might not work in that case.
// These limitation might be removed in future versions of the library.

// Troubleshooting:
// - "undefined reference to chrones::global_coordinator": double-check you called CHRONABLE
// - "undefined reference to chrones::<<anything else>>": double-check you linked with chrones.o

namespace chrones {

////////////////////////////////////////////////////////////////////////////////
// Base tools
////////////////////////////////////////////////////////////////////////////////

class StreamStatistics {
 public:
  StreamStatistics() :
    _count(0),
    _min(std::numeric_limits<float>::max()),
    _max(-std::numeric_limits<float>::max()),
    _sum(),
    _m2n(),
    _samples()
  {}

 public:
  void update(const float x) {
    // Trivial updates
    _min = std::min(_min, x);
    _max = std::max(_max, x);
    _samples.push_back(x);

    // Almost trivial updates but numerically unstable, so we use larger types
    // Variance: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
    const float delta_1 = x - mean();
    // Accumulate in double
    _sum += x;
    // Count in uint64
    _count += 1;
    if (_count >= 2) {
      const float delta_2 = x - mean();
      // Accumulate in double
      _m2n += delta_1 * delta_2;
    }
  }

 public:
  uint64_t count() const { return _count; }

  float mean() const { return _sum / _count; }

  float variance() const { return _m2n / _count; }

  float standard_deviation() const { return std::sqrt(variance()); }

  float min() const { return _min; }

  float median() const {
    if (_samples.empty()) {
      return NAN;
    } else {
      auto nth = _samples.begin() + _samples.size() / 2;
      std::nth_element(_samples.begin(), nth, _samples.end());
      return *nth;
    }
  }

  float max() const { return _max; }

  float sum() const { return _sum; }

 private:
  uint64_t _count;
  float _min;
  float _max;
  double _sum;
  double _m2n;

  // Temporary, for median, until we
  // @todo implement binapprox (https://www.stat.cmu.edu/~ryantibs/median/)
  mutable std::vector<float> _samples;
};

// The default CSV dialect used by Python's `csv` module interprets two double-quote characters
// as a single one inside a double-quoted string. This function replaces each double-quote
// character by two double-quote characters, and puts the result inside double-quotes.
inline std::string quote_for_csv(std::string s) {
  size_t start_pos = 0;
  while ((start_pos = s.find('"', start_pos)) != std::string::npos) {
    s.replace(start_pos, 1, "\"\"");
    start_pos += 2;
  }

  return "\"" + s + "\"";
}

template<class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

////////////////////////////////////////////////////////////////////////////////
// Core: events and coordinator
////////////////////////////////////////////////////////////////////////////////

class Event {
 public:
  Event(
    const std::size_t thread_id_,
    const int64_t time_) :
      thread_id(thread_id_),
      time(time_) {}

  virtual ~Event() {}

 public:
  friend std::ostream& operator<<(std::ostream&, const Event&);
  virtual void output_attributes(std::ostream&) const = 0;

 private:
  std::size_t thread_id;
  int64_t time;
};

inline std::ostream& operator<<(std::ostream& oss, const Event& event) {
  oss << event.thread_id << ',' << event.time;
  event.output_attributes(oss);
  return oss;
}

class StopwatchStartPlainEvent : public Event {
 public:
  StopwatchStartPlainEvent(
    const std::size_t thread_id_,
    const int64_t time_,
    const char* function_) :
      Event(thread_id_, time_),
      function(function_) {}

  StopwatchStartPlainEvent(const StopwatchStartPlainEvent&) = default;
  StopwatchStartPlainEvent(StopwatchStartPlainEvent&&) = default;
  StopwatchStartPlainEvent& operator=(const StopwatchStartPlainEvent&) = default;
  StopwatchStartPlainEvent& operator=(StopwatchStartPlainEvent&&) = default;

 private:
  void output_attributes(std::ostream& oss) const override {
    oss << ",sw_start," << quote_for_csv(function) << ",-,-";
  }

 private:
  const char* function;
};

class StopwatchStartLabelledEvent : public Event {
 public:
  StopwatchStartLabelledEvent(
    const std::size_t thread_id_,
    const int64_t time_,
    const char* function_,
    const char* label_) :
      Event(thread_id_, time_),
      function(function_),
      label(label_) {}

  StopwatchStartLabelledEvent(const StopwatchStartLabelledEvent&) = default;
  StopwatchStartLabelledEvent(StopwatchStartLabelledEvent&&) = default;
  StopwatchStartLabelledEvent& operator=(const StopwatchStartLabelledEvent&) = default;
  StopwatchStartLabelledEvent& operator=(StopwatchStartLabelledEvent&&) = default;

 private:
  void output_attributes(std::ostream& oss) const override {
    oss << ",sw_start," << quote_for_csv(function) << ',' << quote_for_csv(label) << ",-";
  }

 private:
  const char* function;
  const char* label;
};

class StopwatchStartFullEvent : public Event {
 public:
  StopwatchStartFullEvent(
    const std::size_t thread_id_,
    const int64_t time_,
    const char* function_,
    const char* label_,
    const int index_) :
      Event(thread_id_, time_),
      function(function_),
      label(label_),
      index(index_) {}

  StopwatchStartFullEvent(const StopwatchStartFullEvent&) = default;
  StopwatchStartFullEvent(StopwatchStartFullEvent&&) = default;
  StopwatchStartFullEvent& operator=(const StopwatchStartFullEvent&) = default;
  StopwatchStartFullEvent& operator=(StopwatchStartFullEvent&&) = default;

 private:
  void output_attributes(std::ostream& oss) const override {
    oss << ",sw_start," << quote_for_csv(function) << ',' << quote_for_csv(label) << ',' << index;
  }

 private:
  const char* function;
  const char* label;
  int index;
};

class StopwatchStopEvent : public Event {
 public:
  StopwatchStopEvent(
    const std::size_t thread_id_,
    const int64_t time_) :
      Event(thread_id_, time_) {}

 private:
  void output_attributes(std::ostream& oss) const override {
    oss << ",sw_stop";
  }
};

class StopwatchSummaryEvent : public Event {
 public:
  StopwatchSummaryEvent(
    const std::size_t thread_id_,
    const int64_t time_,
    const char* function_,
    const char* label_,
    const uint64_t count_,
    const float mean_,
    const float standard_deviation_,
    const float min_,
    const float median_,
    const float max_,
    const float sum_) :
      Event(thread_id_, time_),
      function(function_),
      label(label_),
      count(count_),
      mean(mean_),
      standard_deviation(standard_deviation_),
      min(min_),
      median(median_),
      max(max_),  // NOLINT(build/include_what_you_use)
      sum(sum_) {}

  StopwatchSummaryEvent(const StopwatchSummaryEvent&) = default;
  StopwatchSummaryEvent(StopwatchSummaryEvent&&) = default;
  StopwatchSummaryEvent& operator=(const StopwatchSummaryEvent&) = default;
  StopwatchSummaryEvent& operator=(StopwatchSummaryEvent&&) = default;

 private:
  void output_attributes(std::ostream& oss) const override {
    oss
      << ",sw_summary," << quote_for_csv(function)
      << ',' << (label == nullptr ? "-" : quote_for_csv(label))
      << ',' << count
      << ',' << static_cast<int64_t>(mean)
      << ',' << static_cast<int64_t>(standard_deviation)
      << ',' << static_cast<int64_t>(min)
      << ',' << static_cast<int64_t>(median)
      << ',' << static_cast<int64_t>(max)
      << ',' << static_cast<int64_t>(sum);
  }

 private:
  const char* function;
  const char* label;
  uint64_t count;
  float mean;
  float standard_deviation;
  float min;
  float median;
  float max;
  float sum;
};

template<typename Info>
class coordinator_tmpl {
 public:
  explicit coordinator_tmpl(std::ostream& stream) :
    _stream(stream),
    _events(),
    _events_mutex(),
    _statistics(),
    _statistics_mutex(),
    _work_done(false),
    _worker(&coordinator_tmpl<Info>::work, this) {}

  ~coordinator_tmpl() {
    _work_done = true;
    _worker.join();
    add_summary_events();
    flush_events();
  }

 public:
  // @todo Add logs of level TRACE
  void start_heavy_stopwatch(
    const char* function
  ) {
    const int64_t start_time = Info::get_time();
    add_event(std::move(make_unique<StopwatchStartPlainEvent>(
      Info::get_thread_id(),
      start_time,
      function)));
  }

  void start_heavy_stopwatch(
    const char* function,
    const char* label
  ) {
    const int64_t start_time = Info::get_time();
    add_event(std::move(make_unique<StopwatchStartLabelledEvent>(
      Info::get_thread_id(),
      start_time,
      function,
      label)));
  }

  void start_heavy_stopwatch(
    const char* function,
    const char* label,
    const int index
  ) {
    const int64_t start_time = Info::get_time();
    add_event(std::move(make_unique<StopwatchStartFullEvent>(
      Info::get_thread_id(),
      start_time,
      function,
      label,
      index)));
  }

  void stop_heavy_stopwatch() {
    const int64_t stop_time = Info::get_time();
    add_event(std::move(make_unique<StopwatchStopEvent>(
      Info::get_thread_id(),
      stop_time)));
  }

  int64_t start_light_stopwatch() {
    return Info::get_time();
  }

  void stop_light_stopwatch(
    const char* function,
    int64_t start_time
  ) {
    const int64_t stop_time = Info::get_time();
    accumulate(function, nullptr, stop_time - start_time);
  }

  void stop_light_stopwatch(
    const char* function,
    const char* label,
    int64_t start_time
  ) {
    const int64_t stop_time = Info::get_time();
    accumulate(function, label, stop_time - start_time);
  }

 private:
  void add_summary_events() {
    const std::size_t thread_id = Info::get_thread_id();
    const int64_t stop_time = Info::get_time();
    boost::lock_guard<boost::mutex> guard(_statistics_mutex);
    for (const auto& stat : _statistics) {
      const char* function;
      const char* label;
      std::tie(function, label) = stat.first;
      add_event(std::move(make_unique<StopwatchSummaryEvent>(
        thread_id,
        stop_time,
        function,
        label,
        stat.second.count(),
        stat.second.mean(),
        stat.second.standard_deviation(),
        stat.second.min(),
        stat.second.median(),
        stat.second.max(),
        stat.second.sum())));
    }
  }

  void accumulate(
      const char* function,
      const char* label,
      const int64_t duration) {
    boost::lock_guard<boost::mutex> guard(_statistics_mutex);
    _statistics[std::make_tuple(function, label)].update(duration);
  }

  void add_event(std::unique_ptr<Event> event) {
    boost::lock_guard<boost::mutex> guard(_events_mutex);
    _events.push_back(std::move(event));
  }

  void work() {
    // Beware, this loop may not even be run once, if the coordinator is destroyed quickly.
    // This is why we call 'flush_events' in the destructor after joining the '_worker'.
    while (!_work_done) {
      flush_events();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Avoid using 100% CPU
    }
  }

  void flush_events() {
    std::vector<std::unique_ptr<Event>> events;
    {
      boost::lock_guard<boost::mutex> guard(_events_mutex);
      if (_events.empty()) {
        return;
      }
      std::swap(events, _events);
    }

    const int process_id = Info::get_process_id();

    for (auto& event : events) {
      _stream << process_id << ',' << *event << '\n';  // No std::endl: don't flush each line, improve performance
    }
  }

 private:
  std::ostream& _stream;

  std::vector<std::unique_ptr<Event>> _events;
  boost::mutex _events_mutex;

  std::map<std::tuple<const char*, const char*>, StreamStatistics> _statistics;
  boost::mutex _statistics_mutex;

  std::atomic_bool _work_done;
  // Using boost::thread instead of std::thread to avoid this segmentation fault:
  // https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
  boost::thread _worker;  // Keep _worker last: all other members must be fully constructed before it starts
};

////////////////////////////////////////////////////////////////////////////////
// Interface: stopwatches and macros
////////////////////////////////////////////////////////////////////////////////

template<typename Info>
class heavy_stopwatch_tmpl {
 public:
  heavy_stopwatch_tmpl(
      coordinator_tmpl<Info>* coordinator,
      const char* function) :
        _coordinator(coordinator) {
    _coordinator->start_heavy_stopwatch(function);
  }

  heavy_stopwatch_tmpl(
      coordinator_tmpl<Info>* coordinator,
      const char* function,
      const char* label) :
        _coordinator(coordinator) {
    _coordinator->start_heavy_stopwatch(function, label);
  }

  heavy_stopwatch_tmpl(
      coordinator_tmpl<Info>* coordinator,
      const char* function,
      const char* label,
      const int index) :
        _coordinator(coordinator) {
    _coordinator->start_heavy_stopwatch(function, label, index);
  }

  ~heavy_stopwatch_tmpl() {
    _coordinator->stop_heavy_stopwatch();
  }

  heavy_stopwatch_tmpl(const heavy_stopwatch_tmpl&) = default;
  heavy_stopwatch_tmpl(heavy_stopwatch_tmpl&&) = default;
  heavy_stopwatch_tmpl& operator=(const heavy_stopwatch_tmpl&) = default;
  heavy_stopwatch_tmpl& operator=(heavy_stopwatch_tmpl&&) = default;

 private:
  coordinator_tmpl<Info>* _coordinator;
};

template<typename Info>
class plain_light_stopwatch_tmpl {
 public:
  plain_light_stopwatch_tmpl(
    coordinator_tmpl<Info>* coordinator,
    const char* function) :
    _coordinator(coordinator),
    _function(function),
    _start_time(coordinator->start_light_stopwatch())
  {}

  ~plain_light_stopwatch_tmpl() {
    _coordinator->stop_light_stopwatch(_function, _start_time);
  }

  plain_light_stopwatch_tmpl(const plain_light_stopwatch_tmpl&) = default;
  plain_light_stopwatch_tmpl(plain_light_stopwatch_tmpl&&) = default;
  plain_light_stopwatch_tmpl& operator=(const plain_light_stopwatch_tmpl&) = default;
  plain_light_stopwatch_tmpl& operator=(plain_light_stopwatch_tmpl&&) = default;

 private:
  coordinator_tmpl<Info>* _coordinator;
  const char* _function;
  int64_t _start_time;
};

template<typename Info>
class labelled_light_stopwatch_tmpl {
 public:
  labelled_light_stopwatch_tmpl(
    coordinator_tmpl<Info>* coordinator,
    const char* function,
    const char* label) :
      _coordinator(coordinator),
      _function(function),
      _label(label),
      _start_time(coordinator->start_light_stopwatch())
  {}

  ~labelled_light_stopwatch_tmpl() {
    _coordinator->stop_light_stopwatch(_function, _label, _start_time);
  }

  labelled_light_stopwatch_tmpl(const labelled_light_stopwatch_tmpl&) = default;
  labelled_light_stopwatch_tmpl(labelled_light_stopwatch_tmpl&&) = default;
  labelled_light_stopwatch_tmpl& operator=(const labelled_light_stopwatch_tmpl&) = default;
  labelled_light_stopwatch_tmpl& operator=(labelled_light_stopwatch_tmpl&&) = default;

 private:
  coordinator_tmpl<Info>* _coordinator;
  const char* _function;
  const char* _label;
  int64_t _start_time;
};

template<typename Info>
plain_light_stopwatch_tmpl<Info> light_stopwatch_tmpl(
  coordinator_tmpl<Info>* coordinator,
  const char* function
) {
  return plain_light_stopwatch_tmpl<Info>(coordinator, function);
}

template<typename Info>
labelled_light_stopwatch_tmpl<Info> light_stopwatch_tmpl(
  coordinator_tmpl<Info>* coordinator,
  const char* function,
  const char* label
) {
  return labelled_light_stopwatch_tmpl<Info>(coordinator, function, label);
}

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

typedef heavy_stopwatch_tmpl<RealInfo> heavy_stopwatch;

inline plain_light_stopwatch_tmpl<RealInfo> light_stopwatch(
  coordinator_tmpl<RealInfo>* coordinator,
  const char* function
) {
  return plain_light_stopwatch_tmpl<RealInfo>(coordinator, function);
}

inline labelled_light_stopwatch_tmpl<RealInfo> light_stopwatch(
  coordinator_tmpl<RealInfo>* coordinator,
  const char* function,
  const char* label
) {
  return labelled_light_stopwatch_tmpl<RealInfo>(coordinator, function, label);
}

inline labelled_light_stopwatch_tmpl<RealInfo> light_stopwatch(
  coordinator_tmpl<RealInfo>* coordinator,
  const char* function,
  const char* label,
  int
) {
  return labelled_light_stopwatch_tmpl<RealInfo>(coordinator, function, label);
}

typedef coordinator_tmpl<RealInfo> coordinator;

extern coordinator global_coordinator;

}  // namespace chrones

#define CHRONABLE(name) \
  namespace chrones { \
    std::chrono::steady_clock::time_point RealInfo::startup_time = std::chrono::steady_clock::now(); \
\
    std::ofstream global_stream( \
      std::string(name) + "." + std::to_string(::getpid()) + ".chrones.csv", std::ios_base::app); \
\
    coordinator global_coordinator(global_stream); \
  }

#ifdef __CUDA_ARCH__

#define CHRONE(...)

#define MINICHRONE(...)

#else

// Variadic macros that forwards their arguments to the appropriate constructors
#define CHRONE(...) auto chrones_stopwatch_##__line__ = chrones::heavy_stopwatch( \
  &chrones::global_coordinator, __PRETTY_FUNCTION__ \
  __VA_OPT__(,) __VA_ARGS__)  // NOLINT(whitespace/comma)

#define MINICHRONE(...) auto chrones_stopwatch_##__line__ = chrones::light_stopwatch( \
  &chrones::global_coordinator, __PRETTY_FUNCTION__ \
  __VA_OPT__(,) __VA_ARGS__)  // NOLINT(whitespace/comma)

#endif

#endif  // NO_CHRONES

#endif  // CHRONES_HPP_

// Copyright 2022 Laurent Cabaret
// Copyright 2022 Vincent Jacques

#ifndef STREAM_STATISTICS_HPP_
#define STREAM_STATISTICS_HPP_

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <vector>


class StreamStatistics {
 public:
  StreamStatistics() :
    _count(0),
    _min(std::numeric_limits<float>::max()),
    _max(-std::numeric_limits<float>::max()),
    _sum(),
    _m2n()
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

 private:
  uint64_t _count;
  float _min;
  float _max;
  double _sum;
  double _m2n;

  // Temporary, for median, until we implement binapprox (https://www.stat.cmu.edu/~ryantibs/median/)
  mutable std::vector<float> _samples;
};

#endif  // STREAM_STATISTICS_HPP_

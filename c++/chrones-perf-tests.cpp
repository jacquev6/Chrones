// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <gtest/gtest.h>

#include <chrono>  // NOLINT(build/c++11)
#include <iostream>
#include <sstream>

#include "chrones.hpp"


using chrones::coordinator;
using chrones::heavy_stopwatch;
using chrones::quote_for_csv;

#define MILLION 1000000


class Timer {
 public:
  Timer() : start(std::chrono::steady_clock::now()) {}

  std::chrono::steady_clock::duration duration() const {
    return std::chrono::steady_clock::now() - start;
  }

 private:
  std::chrono::steady_clock::time_point start;
};


class HeavyChronesPerformanceTest : public testing::Test {
 protected:
  HeavyChronesPerformanceTest() : oss(), c(oss) {}

  std::ostringstream oss;
  coordinator c;
};

TEST_F(HeavyChronesPerformanceTest, SequentialPlain) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, quote_for_csv(__PRETTY_FUNCTION__));
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialPlain
// 1.03545s
// 0.996033s
// 1.07246s
// 0.908944s
// 1.09625s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (5904 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, quote_for_csv(__PRETTY_FUNCTION__), "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialLabelled
// 1.1297s
// 1.074s
// 1.14781s
// 1.07983s
// 1.03868s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (6919 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, quote_for_csv(__PRETTY_FUNCTION__), "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialFull
// 1.21924s
// 1.23741s
// 1.23812s
// 1.21232s
// 1.24065s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (7583 ms)

TEST_F(HeavyChronesPerformanceTest, ParallelFull) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    #pragma omp parallel for
    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, quote_for_csv(__PRETTY_FUNCTION__), "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.ParallelFull
// 1.31601s
// 0.564695s
// 0.559556s
// 0.561189s
// 0.559s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (5952 ms)

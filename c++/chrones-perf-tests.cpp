// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <gtest/gtest.h>

#include <chrono>  // NOLINT(build/c++11)
#include <iostream>
#include <sstream>

#include "chrones.hpp"


using chrones::coordinator;
using chrones::heavy_stopwatch;

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
      heavy_stopwatch t(&c, __PRETTY_FUNCTION__);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialPlain
// 0.63601s
// 0.572634s
// 0.650765s
// 0.506728s
// 0.646034s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (4707 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, __PRETTY_FUNCTION__, "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialLabelled
// 0.961256s
// 0.521277s
// 0.451055s
// 0.563084s
// 0.532272s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (5206 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialFull
// 1.45723s
// 0.704897s
// 0.923529s
// 0.473836s
// 0.90372s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (7033 ms)

TEST_F(HeavyChronesPerformanceTest, ParallelFull) {
  for (int j = 0; j != 5; ++j) {
    Timer timer;

    #pragma omp parallel for
    for (int i = 0; i != MILLION; ++i) {
      heavy_stopwatch t(&c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    // @todo EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.ParallelFull
// 0.770904s
// 0.56584s
// 0.561584s
// 0.545847s
// 0.565209s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (6134 ms)

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
// 0.809478s
// 0.782697s
// 0.734654s
// 0.834528s
// 0.626781s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (5037 ms)

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
// 1.45906s
// 0.991776s
// 0.805872s
// 1.16935s
// 1.03989s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (7096 ms)

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
// 1.38326s
// 1.29593s
// 1.42896s
// 1.17043s
// 0.975813s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (10056 ms)

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
// 1.65702s
// 0.538118s
// 0.546138s
// 0.533121s
// 0.551498s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (7038 ms)

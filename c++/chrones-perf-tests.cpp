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
// 1.48968s
// 1.44116s
// 1.56582s
// 1.3197s
// 1.54042s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (8186 ms)

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
// 1.9144s
// 2.01019s
// 1.49277s
// 1.60026s
// 1.26676s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (11438 ms)

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
// 2.29978s
// 1.87949s
// 1.77202s
// 1.61973s
// 1.74392s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (11199 ms)

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
// 1.81319s
// 0.539742s
// 0.540837s
// 0.536792s
// 0.538241s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (6767 ms)

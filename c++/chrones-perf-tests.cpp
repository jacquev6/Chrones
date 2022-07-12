// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <gtest/gtest.h>
#include <omp.h>

#include <algorithm>
#include <chrono>  // NOLINT(build/c++11)
#include <iostream>
#include <sstream>

#include "chrones.hpp"


using chrones::coordinator;
using chrones::heavy_stopwatch;

#define REPETITIONS 5
#define STOPWATCHES_PER_REPETITION 1000000
#define THREADS 8
static_assert(STOPWATCHES_PER_REPETITION % THREADS == 0, "THREADS must divide STOPWATCHES_PER_REPETITION to create the same number of stopwatches on each thread");


// Note: 'EXPECT_LE's that compare a duration measured during the test
// with a fixed maximum expected are fragile. But performance is an
// important aspect of Chrones, so I think this is better than nothing.

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
  HeavyChronesPerformanceTest() : oss(), c(new coordinator(oss)) {}

  HeavyChronesPerformanceTest(const HeavyChronesPerformanceTest&) = delete;
  HeavyChronesPerformanceTest& operator=(const HeavyChronesPerformanceTest&) = delete;
  HeavyChronesPerformanceTest(HeavyChronesPerformanceTest&&) = delete;
  HeavyChronesPerformanceTest& operator=(HeavyChronesPerformanceTest&&) = delete;

  ~HeavyChronesPerformanceTest() {
    delete c;
    const std::string s = oss.str();
    const int lines = std::count(s.begin(), s.end(), '\n');
    EXPECT_EQ(
      lines,
      2 /* events per stopwatch */
      * STOPWATCHES_PER_REPETITION
      * REPETITIONS);
  }

  std::ostringstream oss;
  coordinator* c;
};

TEST_F(HeavyChronesPerformanceTest, SequentialPlain) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      heavy_stopwatch t(c, __PRETTY_FUNCTION__);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialPlain
// 0.586311s
// 0.546517s
// 0.614285s
// 0.491795s
// 0.613587s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (5667 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      heavy_stopwatch t(c, __PRETTY_FUNCTION__, "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialLabelled
// 0.473203s
// 0.481833s
// 0.431901s
// 0.545248s
// 0.501991s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (5748 ms)

TEST_F(HeavyChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      heavy_stopwatch t(c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.SequentialFull
// 0.764418s
// 0.78549s
// 0.874056s
// 0.585277s
// 0.850266s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (7370 ms)

TEST_F(HeavyChronesPerformanceTest, ParallelFull) {
  omp_set_num_threads(THREADS);
  ASSERT_EQ(omp_get_num_threads(), 1);

  #pragma omp parallel
  {
    EXPECT_EQ(omp_get_num_threads(), THREADS);

    for (int j = 0; j != REPETITIONS; ++j) {
      Timer timer;

      for (int i = 0; i != STOPWATCHES_PER_REPETITION / THREADS; ++i) {
        heavy_stopwatch t(c, __PRETTY_FUNCTION__, "label", i);
      }

      #pragma omp barrier
      const auto d = timer.duration();
      #pragma omp critical
      {
        std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s ";
      }
      #pragma omp barrier
      #pragma omp master
      std::cerr << std::endl;
      EXPECT_LE(d, std::chrono::seconds(1));
    }
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.ParallelFull
// 0.833734s 0.833744s 0.833734s 0.833734s 0.833807s 0.833812s 0.833809s 0.833819s
// 0.82078s 0.820791s 0.820788s 0.820793s 0.820908s 0.820908s 0.820908s 0.82092s
// 0.800548s 0.800548s 0.800548s 0.800558s 0.800556s 0.800596s 0.800548s 0.800677s
// 0.835826s 0.835826s 0.835833s 0.835816s 0.835874s 0.835947s 0.835948s 0.835947s
// 0.779314s 0.779314s 0.779326s 0.779324s 0.779372s 0.77943s 0.779431s 0.779431s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (7582 ms)

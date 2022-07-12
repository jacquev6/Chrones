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
using chrones::light_stopwatch;

#define REPETITIONS 5
#define STOPWATCHES_PER_REPETITION 1000000
#define THREADS 8

static_assert(
  STOPWATCHES_PER_REPETITION % THREADS == 0,
  "THREADS must divide STOPWATCHES_PER_REPETITION to create the same number of stopwatches on each thread");


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
// 0.340938s
// 0.306131s
// 0.317522s
// 0.440938s
// 0.4058s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (5290 ms)

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
// 0.267886s
// 0.395312s
// 0.447362s
// 0.32896s
// 0.342768s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (5372 ms)

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
// 0.24581s
// 0.406601s
// 0.284519s
// 0.320636s
// 0.326781s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (5260 ms)

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
// 0.63508s 0.635081s 0.635081s 0.635087s 0.635127s 0.635227s 0.635224s 0.635235s
// 0.573015s 0.573015s 0.573025s 0.573013s 0.573023s 0.573067s 0.573065s 0.573118s
// 0.558125s 0.558132s 0.558125s 0.558173s 0.558125s 0.558251s 0.558252s 0.558251s
// 0.586375s 0.586382s 0.586383s 0.586375s 0.586378s 0.58645s 0.58642s 0.586375s
// 0.56918s 0.56918s 0.56919s 0.569185s 0.569188s 0.569227s 0.56928s 0.569284s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (5863 ms)


class LightChronesPerformanceTest : public testing::Test {
 protected:
  LightChronesPerformanceTest() : oss(), c(new coordinator(oss)) {}

  ~LightChronesPerformanceTest() {
    delete c;
    const std::string s = oss.str();
    std::string::size_type end = std::string::npos;
    for (int k = 0; k != 6; ++k) {
      end = s.rfind(',', end - 1);
    }
    std::string::size_type begin = s.rfind(',', end - 1) + 1;
    EXPECT_EQ(
      s.substr(begin, end - begin),
      std::to_string(STOPWATCHES_PER_REPETITION * REPETITIONS));
  }

  LightChronesPerformanceTest(const LightChronesPerformanceTest&) = delete;
  LightChronesPerformanceTest& operator=(const LightChronesPerformanceTest&) = delete;
  LightChronesPerformanceTest(LightChronesPerformanceTest&&) = delete;
  LightChronesPerformanceTest& operator=(LightChronesPerformanceTest&&) = delete;

  std::ostringstream oss;
  coordinator* c;
};

TEST_F(LightChronesPerformanceTest, SequentialPlain) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      light_stopwatch t(c, __PRETTY_FUNCTION__);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] LightChronesPerformanceTest.SequentialPlain
// 0.106611s
// 0.106607s
// 0.107645s
// 0.106317s
// 0.108013s
// [       OK ] LightChronesPerformanceTest.SequentialPlain (626 ms)

TEST_F(LightChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      light_stopwatch t(c, __PRETTY_FUNCTION__, "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] LightChronesPerformanceTest.SequentialLabelled
// 0.127809s
// 0.12759s
// 0.13071s
// 0.127654s
// 0.130021s
// [       OK ] LightChronesPerformanceTest.SequentialLabelled (741 ms)

TEST_F(LightChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      light_stopwatch t(c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}
// [ RUN      ] LightChronesPerformanceTest.SequentialFull
// 0.135067s
// 0.127967s
// 0.12754s
// 0.126688s
// 0.128905s
// [       OK ] LightChronesPerformanceTest.SequentialFull (731 ms)

TEST_F(LightChronesPerformanceTest, ParallelFull) {
  omp_set_num_threads(THREADS);
  ASSERT_EQ(omp_get_num_threads(), 1);

  #pragma omp parallel
  {
    EXPECT_EQ(omp_get_num_threads(), THREADS);

    for (int j = 0; j != REPETITIONS; ++j) {
      Timer timer;

      for (int i = 0; i != STOPWATCHES_PER_REPETITION / THREADS; ++i) {
        light_stopwatch t(c, __PRETTY_FUNCTION__, "label", i);
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
// [ RUN      ] LightChronesPerformanceTest.ParallelFull
// 0.627004s 0.627009s 0.627074s 0.6271s 0.627005s 0.627053s 0.627123s 0.627139s
// 0.473635s 0.473635s 0.473636s 0.473635s 0.473638s 0.47364s 0.473681s 0.473677s
// 0.502704s 0.502707s 0.502711s 0.502705s 0.502704s 0.502718s 0.502705s 0.50275s
// 0.464927s 0.464932s 0.464931s 0.464927s 0.464924s 0.464927s 0.464927s 0.464933s
// 0.461055s 0.461057s 0.461057s 0.461056s 0.461057s 0.46106s 0.461101s 0.461174s
// [       OK ] LightChronesPerformanceTest.ParallelFull (2643 ms)

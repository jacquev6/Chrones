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
      heavy_stopwatch(c, __PRETTY_FUNCTION__);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(HeavyChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      heavy_stopwatch(c, __PRETTY_FUNCTION__, "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(HeavyChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      heavy_stopwatch(c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(HeavyChronesPerformanceTest, ParallelFull) {
  omp_set_num_threads(THREADS);
  ASSERT_EQ(omp_get_num_threads(), 1);

  #pragma omp parallel
  {
    EXPECT_EQ(omp_get_num_threads(), THREADS);

    for (int j = 0; j != REPETITIONS; ++j) {
      Timer timer;

      for (int i = 0; i != STOPWATCHES_PER_REPETITION / THREADS; ++i) {
        heavy_stopwatch(c, __PRETTY_FUNCTION__, "label", i);
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
      light_stopwatch(c, __PRETTY_FUNCTION__);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(LightChronesPerformanceTest, SequentialLabelled) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      light_stopwatch(c, __PRETTY_FUNCTION__, "label");
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(LightChronesPerformanceTest, SequentialFull) {
  for (int j = 0; j != REPETITIONS; ++j) {
    Timer timer;

    for (int i = 0; i != STOPWATCHES_PER_REPETITION; ++i) {
      light_stopwatch(c, __PRETTY_FUNCTION__, "label", i);
    }

    const auto d = timer.duration();
    std::cerr << std::chrono::nanoseconds(d).count() / 1e9 << "s" << std::endl;
    EXPECT_LE(d, std::chrono::seconds(1));
  }
}

TEST_F(LightChronesPerformanceTest, ParallelFull) {
  omp_set_num_threads(THREADS);
  ASSERT_EQ(omp_get_num_threads(), 1);

  #pragma omp parallel
  {
    EXPECT_EQ(omp_get_num_threads(), THREADS);

    for (int j = 0; j != REPETITIONS; ++j) {
      Timer timer;

      for (int i = 0; i != STOPWATCHES_PER_REPETITION / THREADS; ++i) {
        light_stopwatch(c, __PRETTY_FUNCTION__, "label", i);
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

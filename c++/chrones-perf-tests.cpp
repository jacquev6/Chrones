// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <gtest/gtest.h>
#include <omp.h>

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
// 0.830142s
// 0.800492s
// 0.931231s
// 1.21084s
// 0.71145s
// [       OK ] HeavyChronesPerformanceTest.SequentialPlain (4776 ms)

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
// 0.550754s
// 1.24408s
// 1.21078s
// 0.723914s
// 0.978825s
// [       OK ] HeavyChronesPerformanceTest.SequentialLabelled (5067 ms)

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
// 0.963214s
// 1.31816s
// 0.962174s
// 1.47985s
// 1.20032s
// [       OK ] HeavyChronesPerformanceTest.SequentialFull (6187 ms)

TEST_F(HeavyChronesPerformanceTest, ParallelFull) {
  const int threads = 8;
  omp_set_num_threads(threads);
  ASSERT_EQ(omp_get_num_threads(), 1);

  #pragma omp parallel
  {
    EXPECT_EQ(omp_get_num_threads(), threads);

    for (int j = 0; j != 5; ++j) {
      Timer timer;

      for (int i = 0; i != MILLION / threads; ++i) {
        heavy_stopwatch t(&c, __PRETTY_FUNCTION__, "label", i);
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
      // @todo EXPECT_LE(d, std::chrono::seconds(1));
    }
  }
}
// [ RUN      ] HeavyChronesPerformanceTest.ParallelFull
// 1.52929s 1.5293s 1.52929s 1.52934s 1.52939s 1.52939s 1.5294s 1.52941s
// 1.81919s 1.81919s 1.81919s 1.81918s 1.81922s 1.81927s 1.81927s 1.81927s
// 1.69576s 1.69576s 1.69581s 1.69577s 1.69582s 1.69576s 1.69587s 1.69587s
// 2.15055s 2.15057s 2.15057s 2.15057s 2.15061s 2.15066s 2.15067s 2.15066s
// 1.76942s 1.76943s 1.76944s 1.76943s 1.76948s 1.76944s 1.76955s 1.76956s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (9150 ms)

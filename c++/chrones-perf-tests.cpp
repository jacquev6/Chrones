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
// 0.804436s 0.804436s 0.804443s 0.804436s 0.804484s 0.804447s 0.804533s 0.804534s
// 0.804883s 0.804889s 0.804891s 0.804889s 0.804888s 0.804928s 0.804888s 0.804998s
// 0.821919s 0.822003s 0.822003s 0.821998s 0.822004s 0.822003s 0.822004s 0.822014s
// 0.816145s 0.816144s 0.816154s 0.816194s 0.816192s 0.816281s 0.816281s 0.816282s
// 0.818835s 0.818839s 0.818835s 0.818835s 0.81884s 0.818835s 0.818841s 0.818913s
// [       OK ] HeavyChronesPerformanceTest.ParallelFull (7375 ms)

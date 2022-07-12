// Copyright 2022 Laurent Cabaret
// Copyright 2022 Vincent Jacques

#include <gtest/gtest.h>

#include "chrones.hpp"


using chrones::StreamStatistics;

TEST(StreamStatisticsTest, MaxOnNegativeElement) {
  StreamStatistics stats;
  stats.update(-1);

  EXPECT_EQ(stats.max(), -1);
}

TEST(StreamStatisticsTest, MinOnPositiveElement) {
  StreamStatistics stats;
  stats.update(1);

  EXPECT_EQ(stats.min(), 1);
}

TEST(StreamStatisticsTest, AllMetricsOnZeroElements) {
  StreamStatistics stats;

  EXPECT_EQ(stats.count(), 0);
  EXPECT_TRUE(std::isnan(stats.mean()));
  EXPECT_TRUE(std::isnan(stats.variance()));
  EXPECT_TRUE(std::isnan(stats.standard_deviation()));
  EXPECT_EQ(stats.min(), std::numeric_limits<float>::max());
  EXPECT_TRUE(std::isnan(stats.median()));
  EXPECT_EQ(stats.max(), -std::numeric_limits<float>::max());
  EXPECT_EQ(stats.sum(), 0);
}

TEST(StreamStatisticsTest, AllMetricsOnOneElements) {
  StreamStatistics stats;
  stats.update(2);

  EXPECT_EQ(stats.count(), 1);
  EXPECT_EQ(stats.mean(), 2);
  EXPECT_EQ(stats.variance(), 0);
  EXPECT_EQ(stats.standard_deviation(), 0);
  EXPECT_EQ(stats.min(), 2);
  EXPECT_EQ(stats.median(), 2);
  EXPECT_EQ(stats.max(), 2);
  EXPECT_EQ(stats.sum(), 2);
}

TEST(StreamStatisticsTest, AllMetricsOnTwoElements) {
  StreamStatistics stats;
  for (float x : {4, 2}) {
    stats.update(x);
  }

  EXPECT_EQ(stats.count(), 2);
  EXPECT_EQ(stats.mean(), 3);
  EXPECT_EQ(stats.variance(), 1);
  EXPECT_EQ(stats.standard_deviation(), 1);
  EXPECT_EQ(stats.min(), 2);
  EXPECT_EQ(stats.median(), 4);
  EXPECT_EQ(stats.max(), 4);
  EXPECT_EQ(stats.sum(), 6);
}

TEST(StreamStatisticsTest, AllMetricsOnThreeElements) {
  StreamStatistics stats;
  for (float x : {4, 2, 3}) {
    stats.update(x);
  }

  EXPECT_EQ(stats.count(), 3);
  EXPECT_EQ(stats.mean(), 3);
  EXPECT_FLOAT_EQ(stats.variance(), 0.66666669);
  EXPECT_FLOAT_EQ(stats.standard_deviation(), 0.81649661);
  EXPECT_EQ(stats.min(), 2);
  EXPECT_EQ(stats.median(), 3);
  EXPECT_EQ(stats.max(), 4);
  EXPECT_EQ(stats.sum(), 9);
}

TEST(StreamStatisticsTest, AllMetricsOnFourElements) {
  StreamStatistics stats;
  for (float x : {4, 2, 3, 1}) {
    stats.update(x);
  }

  EXPECT_EQ(stats.count(), 4);
  EXPECT_EQ(stats.mean(), 2.5);
  EXPECT_EQ(stats.variance(), 1.25);
  EXPECT_FLOAT_EQ(stats.standard_deviation(), 1.118034);
  EXPECT_EQ(stats.min(), 1);
  EXPECT_EQ(stats.median(), 3);
  EXPECT_EQ(stats.max(), 4);
  EXPECT_EQ(stats.sum(), 10);
}

TEST(StreamStatisticsTest, AllMetricsOnFiveElements) {
  StreamStatistics stats;
  for (float x : {4, 2, 1, 3, 0}) {
    stats.update(x);
  }

  EXPECT_EQ(stats.count(), 5);
  EXPECT_EQ(stats.mean(), 2);
  EXPECT_EQ(stats.variance(), 2);
  EXPECT_FLOAT_EQ(stats.standard_deviation(), 1.4142135);
  EXPECT_EQ(stats.min(), 0);
  EXPECT_EQ(stats.median(), 2);
  EXPECT_EQ(stats.max(), 4);
  EXPECT_EQ(stats.sum(), 10);
}

const unsigned large = 17000000;

TEST(StreamStatisticsTest, AverageOfManyEqualElements_0) {
  StreamStatistics stats;

  // Large enough to loose precision in float arithmetics
  ASSERT_EQ(large + 1.f, large);  // Not FLOAT_EQ

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large; ++i) {
      stats.update(0);
    }

    EXPECT_EQ(stats.mean(), 0);
    EXPECT_EQ(stats.variance(), 0);
  }
}

TEST(StreamStatisticsTest, AverageOfManyEqualElements_1) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large; ++i) {
      stats.update(1);
    }

    EXPECT_EQ(stats.mean(), 1);
    EXPECT_EQ(stats.variance(), 0);
  }
}

TEST(StreamStatisticsTest, AverageOfManyEqualElements_10) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large; ++i) {
      stats.update(10);
    }

    EXPECT_EQ(stats.mean(), 10);
    EXPECT_EQ(stats.variance(), 0);
  }
}

TEST(StreamStatisticsTest, AverageOfManyEqualElements_16) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large; ++i) {
      stats.update(16);
    }

    EXPECT_EQ(stats.mean(), 16);
    EXPECT_EQ(stats.variance(), 0);
  }
}

TEST(StreamStatisticsTest, VarianceOfManyEquidistantElements_1) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large / 2; ++i) {
      stats.update(1);
      stats.update(-1);
    }

    EXPECT_EQ(stats.mean(), 0);
    EXPECT_EQ(stats.variance(), 1);
  }
}

TEST(StreamStatisticsTest, VarianceOfManyEquidistantElements_10) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large / 2; ++i) {
      stats.update(10);
      stats.update(-10);
    }

    EXPECT_EQ(stats.mean(), 0);
    EXPECT_EQ(stats.variance(), 100);
  }
}

TEST(StreamStatisticsTest, VarianceOfManyEquidistantElements_16) {
  StreamStatistics stats;

  for (int j = 0; j != 5; ++j) {
    for (int i = 0; i != large / 2; ++i) {
      stats.update(16);
      stats.update(-16);
    }

    EXPECT_EQ(stats.mean(), 0);
    EXPECT_EQ(stats.variance(), 256);
  }
}

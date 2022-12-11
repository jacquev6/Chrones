// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <gtest/gtest.h>

#include <sstream>

#include "chrones.hpp"

TEST(ChronesTest, QuoteForCsv) {
  EXPECT_EQ(chrones::quote_for_csv("a"), "\"a\"");
  EXPECT_EQ(chrones::quote_for_csv("abc\"def"), "\"abc\"\"def\"");
  EXPECT_EQ(chrones::quote_for_csv("abc\""), "\"abc\"\"\"");
  EXPECT_EQ(chrones::quote_for_csv("\"def"), "\"\"\"def\"");
}

struct MockInfo {
  static int64_t time;

  static int64_t get_time() {
    return time;
  }

  static int process_id;

  static int get_process_id() {
    return process_id;
  }

  static std::size_t thread_id;

  static std::size_t get_thread_id() {
    return thread_id;
  }
};

int64_t MockInfo::time = 0;
int MockInfo::process_id = 0;
std::size_t MockInfo::thread_id = 0;

typedef chrones::heavy_stopwatch_tmpl<MockInfo> heavy_stopwatch;
typedef chrones::coordinator_tmpl<MockInfo> coordinator;

inline chrones::plain_light_stopwatch_tmpl<MockInfo> light_stopwatch(
  coordinator* coordinator,
  const char* function
) {
  return chrones::plain_light_stopwatch_tmpl<MockInfo>(coordinator, function);
}

inline chrones::labelled_light_stopwatch_tmpl<MockInfo> light_stopwatch(
  coordinator* coordinator,
  const char* function,
  const char* label
) {
  return chrones::labelled_light_stopwatch_tmpl<MockInfo>(coordinator, function, label);
}

inline chrones::labelled_light_stopwatch_tmpl<MockInfo> light_stopwatch(
  coordinator* coordinator,
  const char* function,
  const char* label,
  int
) {
  return chrones::labelled_light_stopwatch_tmpl<MockInfo>(coordinator, function, label);
}


TEST(ChronesTest, BasicHeavyOnce) {
  for (int i = 0; i != 5000; ++i) {  // Repeat test to gain confidence about race conditions
    std::ostringstream oss;
    MockInfo::time = 652;
    MockInfo::process_id = 7;
    MockInfo::thread_id = 12;

    {
      coordinator c(oss);
      {
        auto t = heavy_stopwatch(&c, "f");
        MockInfo::time = 694;
      }
      MockInfo::time = 710;
    }

    ASSERT_EQ(
      oss.str(),
      "7,12,652,sw_start,\"f\",-,-\n"
      "7,12,694,sw_stop\n");
  }
}

TEST(ChronesTest, BasicLightOnce) {
  for (int i = 0; i != 5000; ++i) {  // Repeat test to gain confidence about race conditions
    std::ostringstream oss;
    MockInfo::time = 652;
    MockInfo::process_id = 7;
    MockInfo::thread_id = 12;

    {
      coordinator c(oss);
      {
        auto t = light_stopwatch(&c, "f");
        MockInfo::time = 694;
      }
      MockInfo::time = 710;
    }

    ASSERT_EQ(
      oss.str(),
      "7,12,710,sw_summary,\"f\",-,1,42,0,42,42,42,42\n");
  }
}

TEST(ChronesTest, BasicHeavyFewTimes) {
  std::ostringstream oss;
  MockInfo::time = 122;
  MockInfo::process_id = 8;
  MockInfo::thread_id = 1;

  {
    coordinator c(oss);
    for (int i = 1; i != 4; ++i) {
      MockInfo::time += i * 4;
      auto t = heavy_stopwatch(&c, "f", "label", i);
      MockInfo::time += i * 3;
    }
    MockInfo::time = 200;
  }

  ASSERT_EQ(
    oss.str(),
    "8,1,126,sw_start,\"f\",\"label\",1\n"
    "8,1,129,sw_stop\n"
    "8,1,137,sw_start,\"f\",\"label\",2\n"
    "8,1,143,sw_stop\n"
    "8,1,155,sw_start,\"f\",\"label\",3\n"
    "8,1,164,sw_stop\n");
}

TEST(ChronesTest, FlushedBeforeCoordinatorDestruction) {
  std::ostringstream oss;
  MockInfo::time = 0;
  MockInfo::process_id = 0;
  MockInfo::thread_id = 0;

  coordinator c(oss);
  {
    auto t = heavy_stopwatch(&c, "f");
  }

  // Just wait
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Data arrives in oss *before* c in destroyed
  ASSERT_EQ(
    oss.str(),
    "0,0,0,sw_start,\"f\",-,-\n"
    "0,0,0,sw_stop\n");
}

TEST(ChronesTest, BasicLightFewTimes) {
  std::ostringstream oss;
  MockInfo::time = 122;
  MockInfo::process_id = 8;
  MockInfo::thread_id = 1;

  {
    coordinator c(oss);
    for (int i = 1; i != 4; ++i) {
      MockInfo::time += i * 4;
      auto t = light_stopwatch(&c, "f", "l", i);
      MockInfo::time += i * 3;
    }
    MockInfo::time = 200;
  }

  ASSERT_EQ(
    oss.str(),
    "8,1,200,sw_summary,\"f\",\"l\",3,6,2,3,6,9,18\n");
}

TEST(ChronesTest, LabelWithQuotes) {
  std::ostringstream oss;
  MockInfo::time = 0;
  MockInfo::process_id = 0;
  MockInfo::thread_id = 0;

  {
    coordinator c(oss);
    auto t = heavy_stopwatch(&c, "f", "a 'label' with \"quotes\"");
  }

  ASSERT_EQ(
    oss.str(),
    "0,0,0,sw_start,\"f\",\"a 'label' with \"\"quotes\"\"\",-\n"
    "0,0,0,sw_stop\n");
}

TEST(ChronesTest, Index) {
  std::ostringstream oss;
  MockInfo::time = 0;
  MockInfo::process_id = 0;
  MockInfo::thread_id = 0;

  {
    coordinator c(oss);
    auto t = heavy_stopwatch(&c, "f", "label", 42);
  }

  ASSERT_EQ(
    oss.str(),
    "0,0,0,sw_start,\"f\",\"label\",42\n"
    "0,0,0,sw_stop\n");
}

TEST(ChronesTest, NullCoordinator) {
  // These are all no-ops, so we just check for bad memory accesses
  heavy_stopwatch(nullptr, "name");
  heavy_stopwatch(nullptr, "name", "label");
  heavy_stopwatch(nullptr, "name", "label", 42);
  light_stopwatch(nullptr, "name");
  light_stopwatch(nullptr, "name", "label");
  light_stopwatch(nullptr, "name", "label", 42);
}

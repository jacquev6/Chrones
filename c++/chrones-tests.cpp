// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#ifndef NO_CHRONES

#include <gtest/gtest.h>

#include <sstream>

#include "chrones.hpp"


CHRONABLE("chrones-tests")

namespace foo {

struct Bar {
  static void actual_file_g(int, float, std::pair<int, float>*) {
    CHRONE();
  }
};

}

void actual_file_h() {
  CHRONE();
}

void actual_file_f() {
  CHRONE();

  #pragma omp parallel for
  for (int i = 0; i < 6; ++i) {
    CHRONE("loop a", i);
    foo::Bar::actual_file_g(42, 57, nullptr);
  }

  #pragma omp parallel for
  for (int i = 0; i < 8; ++i) {
    CHRONE("loop b", i);
    actual_file_h();
  }
}

TEST(ChronesTest, ActualFile) {
  // This test doesn't actually check anything. It's used only to generate a `.chrones.csv` file for
  // testing `chrones.py report`.
  CHRONE();
  {
    CHRONE("intermediate 'wwwwww'");
    actual_file_f();
  }
  {
    CHRONE("intermediate, \"xxxxxx\"");
  }
}

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

typedef chrones::stopwatch_tmpl<MockInfo> stopwatch;
typedef chrones::coordinator_tmpl<MockInfo> coordinator;


TEST(ChronesTest, BasicOnce) {
  for (int i = 0; i != 5000; ++i) {  // Repeat test to gain confidence about race conditions
    std::ostringstream oss;
    MockInfo::time = 652;
    MockInfo::process_id = 7;
    MockInfo::thread_id = 12;

    {
      coordinator c(oss);
      {
        stopwatch t(&c, "f");
        MockInfo::time = 694;
      }
      MockInfo::time = 710;
    }

    ASSERT_EQ(
      oss.str(),
      "7,12,652,sw_start,f,-,-\n"
      "7,12,694,sw_stop\n"
      "7,12,710,sw_summary,f,-,1,42,0,42,42,42,42\n");
  }
}

TEST(ChronesTest, BasicFewTimes) {
    std::ostringstream oss;
    MockInfo::time = 122;
    MockInfo::process_id = 8;
    MockInfo::thread_id = 1;

    {
      coordinator c(oss);
      for (int i = 1; i != 4; ++i) {
        MockInfo::time += i * 4;
        stopwatch t(&c, "f", boost::none, i);
        MockInfo::time += i * 3;
      }
      MockInfo::time = 200;
    }

    ASSERT_EQ(
      oss.str(),
      "8,1,126,sw_start,f,-,1\n"
      "8,1,129,sw_stop\n"
      "8,1,137,sw_start,f,-,2\n"
      "8,1,143,sw_stop\n"
      "8,1,155,sw_start,f,-,3\n"
      "8,1,164,sw_stop\n"
      "8,1,200,sw_summary,f,-,3,6,2,3,6,9,18\n");
}

TEST(ChronesTest, LabelWithQuotes) {
  std::ostringstream oss;
  MockInfo::time = 0;
  MockInfo::process_id = 0;
  MockInfo::thread_id = 0;

  {
    coordinator c(oss);
    stopwatch t(&c, "f", "a 'label' with \"quotes\"");
  }

  ASSERT_EQ(
    oss.str(),
    "0,0,0,sw_start,f,\"a 'label' with \"\"quotes\"\"\",-\n"
    "0,0,0,sw_stop\n"
    "0,0,0,sw_summary,f,\"a 'label' with \"\"quotes\"\"\",1,0,0,0,0,0,0\n");
}

TEST(ChronesTest, Index) {
  std::ostringstream oss;
  MockInfo::time = 0;
  MockInfo::process_id = 0;
  MockInfo::thread_id = 0;

  {
    coordinator c(oss);
    stopwatch t(&c, "f", "label", 42);
  }

  ASSERT_EQ(
    oss.str(),
    "0,0,0,sw_start,f,\"label\",42\n"
    "0,0,0,sw_stop\n"
    "0,0,0,sw_summary,f,\"label\",1,0,0,0,0,0,0\n");
}

#endif  // NO_CHRONES

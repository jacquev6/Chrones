// Copyright 2020-2021 Laurent Cabaret
// Copyright 2020-2021 Vincent Jacques

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
  for (int i = 0; i < 4; ++i) {
    CHRONE("loop a", i);
    foo::Bar::actual_file_g(42, 57, nullptr);
  }

  #pragma omp parallel for
  for (int i = 0; i < 4; ++i) {
    CHRONE("loop b", i);
    actual_file_h();
  }
}

TEST(TestChrones, ActualFile) {
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

TEST(TestChrones, QuoteForCsv) {
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


TEST(TestChrones, Basic) {
  for (int i = 0; i != 5'000; ++i) {  // Repeat test to gain confidence about race conditions
    std::ostringstream oss;
    MockInfo::time = 652;
    MockInfo::process_id = 7;
    MockInfo::thread_id = 12;

    {
      coordinator c(oss);
      stopwatch t(&c, "f");
      MockInfo::time = 789;
    }

    ASSERT_EQ(oss.str(), "7,12,652,sw_start,f,-,-\n7,12,789,sw_stop\n");
  }
}

TEST(TestChrones, LabelWithQuotes) {
  std::ostringstream oss;
  MockInfo::time = 1222;
  MockInfo::process_id = 5;
  MockInfo::thread_id = 57;

  {
    coordinator c(oss);
    stopwatch t(&c, "f", "a 'label' with \"quotes\"");
    MockInfo::time = 1712;
  }

  ASSERT_EQ(
    oss.str(),
    "5,57,1222,sw_start,f,\"a 'label' with \"\"quotes\"\"\",-\n5,57,1712,sw_stop\n");
}

TEST(TestChrones, Index) {
  std::ostringstream oss;
  MockInfo::time = 788;
  MockInfo::process_id = 6;
  MockInfo::thread_id = 63;

  {
    coordinator c(oss);
    stopwatch t(&c, "f", "label", 42);
    MockInfo::time = 912;
  }

  ASSERT_EQ(oss.str(), "6,63,788,sw_start,f,\"label\",42\n6,63,912,sw_stop\n");
}

#endif  // NO_CHRONES

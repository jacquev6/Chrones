#include <gtest/gtest.h>
#include "chrone.hpp"

TEST(TestChrone, sizeOfStable) {
    chrone leChrone("test_chrones.csv");
    {
    timer test_timer("TestTimerFn0", &leChrone);
    timer test_timer2("TestTimerFn1", &leChrone);
    timer test_timer3("TestTimerFn2", &leChrone);
    }
    EXPECT_EQ(leChrone.getSize(), 3);
}

// This is a mock, used to replace the usual `std::chrono::high_resolution_clock`
// in the tests. Basically, it's a way to make `clk` return known values to check
// the behavior of our code with these values.
// We build this mock ourself, but it might be easier or more reusable
// to use [GMock](https://github.com/google/googletest/tree/master/googlemock)
struct mock_clock {
    typedef std::chrono::microseconds duration;
    typedef std::chrono::time_point<mock_clock> time_point;

    static time_point now() {
        return time_point(duration(*(next_value++)));
    }

    static std::vector<int>::const_iterator next_value;
};

std::vector<int>::const_iterator mock_clock::next_value;

typedef timer_<mock_clock> test_timer;

TEST(TestChrone, elapsedOneIteration) {
    chrone c("TestChrone.elapsedOneIteration");

    // We expect two calls to clk::now, and will return the following values:
    std::vector<int> now_values { 10, 52 };
    mock_clock::next_value = now_values.begin();

    {
        test_timer("a", &c);
    }

    // We check that all values were returned
    EXPECT_EQ(mock_clock::next_value, now_values.end());
    // And check that their difference is returned as duration
    EXPECT_EQ(c.getDuration("a"), 42);
}

TEST(TestChrone, elapsedSevenIterations) {
    chrone c("TestChrone.elapsedSevenIterations");

    std::vector<int> now_values { 10, 52 };
    mock_clock::next_value = now_values.begin();

    {
        test_timer("a", &c, 7);
    }

    EXPECT_EQ(mock_clock::next_value, now_values.end());
    // And check that their difference, divide by 7, is returned as duration
    EXPECT_EQ(c.getDuration("a"), 6);
}

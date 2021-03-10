#include <gtest/gtest.h>
#include <time.h>
#include <sched.h>
#include "chrone.hpp"

TEST(TestChrone, TimerAdd) {
    chrone leChrone;
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    }
    EXPECT_EQ(leChrone.getLabelOfTimer(0), "DummyFunctionTimer");
}

TEST(TestChrone, sizeOfStable) {
    chrone leChrone;
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    timer test_timer2("DummyFunctionTimer", &leChrone);
    timer test_timer3("DummyFunctionTimer", &leChrone);
    }
    EXPECT_EQ(leChrone.getSize(), 3);
}

TEST(TestDestructorDelay, 0) {
    chrone leChrone;
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), 0, 250);
}

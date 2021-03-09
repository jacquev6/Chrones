#include <gtest/gtest.h>
#include <unistd.h>
#include "chrone.hpp"

/* To solve: nothing is in the chrone while timer is not destructed
TEST(TestChrone, TimerAdd) {
    chrone leChrone;
    timer test_timer("DummyFunctionTimer", &leChrone);
    EXPECT_EQ(leChrone.getLabelOfTimer(0), "DummyFunctionTimer");
}
*/ 

TEST(TestChrone, TimerAdd) {
    chrone leChrone;
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    }
    EXPECT_EQ(leChrone.getLabelOfTimer(0), "DummyFunctionTimer");
}

TEST(TestChroneTime, LongElapsedTime_LowExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000000; //1s
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.01*delay);
}

TEST(TestChroneTime, LongElapsedTime_HigherExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000000; //1s
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.001*delay);
}


TEST(TestChroneTime, MediumElapsedTime_LowExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000; //1ms
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.01*delay);
}

TEST(TestChroneTime, MediumElapsedTime_HigherExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000; //1ms
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.001*delay);
}

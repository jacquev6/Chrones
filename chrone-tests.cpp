#include <gtest/gtest.h>
#include <unistd.h>
#include "chrone.hpp"

TEST(TestTimer, Constructor) {
    chrone leChrone;
    timer test_timer("DummyFunctionTimer", &leChrone);
    EXPECT_EQ(test_timer.getLabel(), "DummyFunctionTimer");
}

TEST(TestChrone, LongElapsedTime_LowExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000000; //1s
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.01*delay);
}

TEST(TestChrone, LongElapsedTime_HigherExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000000; //1s
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.001*delay);
}


TEST(TestChrone, MediumElapsedTime_LowExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000; //1ms
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.01*delay);
}

TEST(TestChrone, MediumElapsedTime_HigherExpectations) {
    chrone leChrone;
    unsigned long int delay = 1000; //1ms
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    usleep(delay);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0)/1000, delay, 0.0001*delay);
}

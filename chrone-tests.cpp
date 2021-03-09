#include <gtest/gtest.h>
#include <unistd.h>
#include <time.h>
#include "chrone.hpp"

/* To solve: nothing is in the chrone while timer is not destructed
TEST(TestChrone, TimerAdd) {
    chrone leChrone;
    nanosleep(&delay, NULL);
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

TEST(TestChrone, sizeOfStable) {
    chrone leChrone;
    {
    timer test_timer("DummyFunctionTimer", &leChrone);
    timer test_timer2("DummyFunctionTimer", &leChrone);
    timer test_timer3("DummyFunctionTimer", &leChrone);
    }
    EXPECT_EQ(leChrone.getSize(), 3);
}



TEST(TestChroneTime, LongElapsedTime_LowExpectations) {
    chrone leChrone;
    long int delay_value = 1000000000L;
    const timespec delay = {delay_value/1000000000L, 0};
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
        nanosleep(&delay, NULL);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), delay_value, 0.01*delay_value);
}

TEST(TestChroneTime, LongElapsedTime_HigherExpectations) {
    chrone leChrone;
    long int delay_value = 1000000000L;
    const timespec delay = {delay_value/1000000000L, 0};
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
        nanosleep(&delay, NULL);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), delay_value, 0.001*delay_value);
}


TEST(TestChroneTime, MediumElapsedTime_LowExpectations) {
    chrone leChrone;
    long int delay_value = 1000000L;
    const timespec delay = {0, delay_value};
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
    nanosleep(&delay, NULL);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), delay_value, 0.01*delay_value);
}

TEST(TestChroneTime, MediumElapsedTime_HigherExpectations) {
    chrone leChrone;
    long int delay_value = 1000000L;
    const timespec delay = {0, delay_value};
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
    nanosleep(&delay, NULL);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), delay_value, 0.001*delay_value);
}


TEST(TestDestructorDelay, 0) {
    chrone leChrone;
    long int delay_value = 1L;
    const timespec delay = {0, delay_value};
    {
        timer test_timer("DummyFunctionTimer", &leChrone);
    nanosleep(&delay, NULL);
    }
    EXPECT_NEAR(leChrone.getTimeOfTimer(0), delay_value, delay_value);
}

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

#include <gtest/gtest.h>

#include "chrone.hpp"

TEST(TestChrone, Constructor) {
    chrone leChrone;
    timer test_timer("DummyFunctionTimer", &leChrone);
    EXPECT_EQ(test_timer.getLabel(), "DummyFunctionTimer");
}
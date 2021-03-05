#include <gtest/gtest.h>

#include "chrone.hpp"

TEST(TestChrone, Constructor) {
    timer test_timer("DummyFunctionTimer");
    EXPECT_EQ(test_timer.getLabel(), "DummyFunctionTimer");
}
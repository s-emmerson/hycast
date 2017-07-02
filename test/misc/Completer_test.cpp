/**
 * This file tests the class `Completer`.
 *
 * Copyright 2017 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Completer_test.cpp
 * @author: Steven R. Emmerson
 */


#include "Completer.h"

#include <array>
#include <gtest/gtest.h>

namespace {

// The fixture for testing class Completer.
class CompleterTest : public ::testing::Test {
};

// Tests construction of void completer
TEST_F(CompleterTest, VoidConstruction) {
    hycast::Completer<void> completer{};
}

// Tests construction of int completer
TEST_F(CompleterTest, IntConstruction) {
    hycast::Completer<int> completer{};
}

// Tests execution of void task
TEST_F(CompleterTest, VoidExecution) {
    hycast::Completer<void> completer{};
    auto future1 = completer.submit([]{});
    auto future2 = completer.get();
    EXPECT_TRUE(future1 == future2);
    EXPECT_FALSE(future2.wasCanceled());
    EXPECT_NO_THROW(future2.getResult());
}

// Tests execution of int task
TEST_F(CompleterTest, IntExecution) {
    hycast::Completer<int> completer{};
    auto future1 = completer.submit([]{return 1;});
    auto future2 = completer.get();
    EXPECT_TRUE(future1 == future2);
    EXPECT_FALSE(future2.wasCanceled());
    EXPECT_EQ(1, future2.getResult());
}

// Tests execution of multiple void tasks
TEST_F(CompleterTest, MultipleVoidExecution) {
    hycast::Completer<void> completer{};
    std::array<hycast::Future<void>, 8> futures;
    for (unsigned i = 0; i < futures.size(); ++i) {
        auto future = completer.submit([]{});
        futures[i] = future;
    }
    for (unsigned i = 0; i < futures.size(); ++i) {
        auto future = completer.get();
        EXPECT_FALSE(future.wasCanceled());
        EXPECT_NO_THROW(future.getResult());
    }
}

// Tests execution of multiple int tasks
TEST_F(CompleterTest, MultipleIntExecution) {
    hycast::Completer<int> completer{};
    std::array<hycast::Future<int>, 8> futures;
    for (unsigned i = 0; i < futures.size(); ++i) {
        auto future = completer.submit([i]{return i;});
        futures[i] = future;
    }
    for (unsigned i = 0; i < futures.size(); ++i) {
        auto future = completer.get();
        EXPECT_FALSE(future.wasCanceled());
        int j = future.getResult();
        EXPECT_TRUE(futures[j] == future);
    }
}

// Tests cancellation of void task
TEST_F(CompleterTest, VoidCancellation) {
    hycast::Completer<void> completer{};
    auto future = completer.submit([]{::pause();});
    future.cancel();
    EXPECT_TRUE(future.wasCanceled());
    EXPECT_THROW(future.getResult(), hycast::LogicError);
}

// Tests cancellation of int task
TEST_F(CompleterTest, IntCancellation) {
    hycast::Completer<int> completer{};
    auto future = completer.submit([]{::pause(); return 1;});
    future.cancel();
    EXPECT_TRUE(future.wasCanceled());
    EXPECT_THROW(future.getResult(), hycast::LogicError);
}

// Tests destruction with active task
TEST_F(CompleterTest, DestructionWithTask) {
    hycast::Completer<void> completer{};
    completer.submit([]{::pause();});
}

// Tests destruction with active future
TEST_F(CompleterTest, DestructionWithFuture) {
    hycast::Completer<void> completer{};
    auto future = completer.submit([]{::pause();});
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

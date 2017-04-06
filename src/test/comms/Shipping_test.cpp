/**
 * This file tests the class `Shipping`.
 *
 * Copyright 2017 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Shipping.cpp
 * @author: Steven R. Emmerson
 */

#include "Shipping.h"

#include <gtest/gtest.h>

namespace {

// The fixture for testing class Shipping.
class ShippingTest : public ::testing::Test {
protected:
};

// Tests construction
TEST_F(ShippingTest, Construction) {
    hycast::Shipping();
}

// Tests shipping a product
TEST_F(ShippingTest, Shipping) {
    hycast::ProdIndex prodIndex(0);
    char              data[128000];
    ::memset(data, 0xbd, sizeof(data));
    hycast::Product   prod("product", prodIndex, data, sizeof(data));
    hycast::Shipping  shipping{};
    shipping.ship(prod);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

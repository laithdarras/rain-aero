#include "greeter.h"

#include <gtest/gtest.h>

namespace
{

TEST(Greet, DefaultsToHelloWorld)
{
  EXPECT_EQ(quickstart::greet("world"), "Hello, world!");
}

TEST(Greet, AcceptsAnyName)
{
  EXPECT_EQ(quickstart::greet("Rain"), "Hello, Rain!");
}

}  // namespace

// Catch2 recommends placing these lines by themselves in a translation unit
// which will help reduce unnecessary recompilations of the expensive Catch main
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "simplnx/UnitTest/OwnedCatchRunner.hpp"

int main(int argc, char* argv[])
{
  return nx::core::UnitTest::RunOwnedCatchSession(argc, argv);
}

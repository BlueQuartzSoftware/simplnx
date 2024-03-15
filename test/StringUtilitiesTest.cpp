#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include string

using namespace nx::core;

TEST_CASE("Utility Function Test: split(str, char)")
{
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result1 = StringUtilities::split(inputStr, '|');

  REQUIRE(result1.size() == 5);

  // Case 2
  inputStr
}

#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include string

using namespace nx::core;

TEST_CASE("Utility Function Test: split(str, char)")
{
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = StringUtilities::split(inputStr, '|');

  REQUIRE(result.size() == 5);

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|"

  result = StringUtilities::split(inputStr, '|');

  REQUIRE(result.size() == 5);

  // Case 3
  inputStr = "|This|Is||A||Baseline|Test||"

  result = StringUtilities::split(inputStr, '|');

  REQUIRE(result.size() == 5);
}

TEST_CASE("Utility Function Test: split(str, char, bool)")
{
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = StringUtilities::split(inputStr, '|', false);

  REQUIRE(result.size() == 5);

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|"

  result = StringUtilities::split(inputStr, '|', false);

  REQUIRE(result.size() == 5);

  // Case 3
  inputStr = "|This|Is||A||Baseline|Test||"

  result = StringUtilities::split(inputStr, '|', false);

  REQUIRE(result.size() == 5);

  // Case 4
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = StringUtilities::split(inputStr, '|', true);

  REQUIRE(result.size() == 5);

  // Case 5
  inputStr = "|This|Is|A|Baseline|Test|"

  result = StringUtilities::split(inputStr, '|', true);

  REQUIRE(result.size() == 5);

  // Case 6
  inputStr = "|This|Is||A||Baseline|Test||"

  result = StringUtilities::split(inputStr, '|', true);

  REQUIRE(result.size() == 8);
}

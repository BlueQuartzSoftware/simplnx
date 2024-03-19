#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;

TEST_CASE("Utility Function Test: split(str, char)")
{
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = StringUtilities::split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|";

  result = StringUtilities::split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  inputStr = "||This|Is||A||Baseline|Test||";

  result = StringUtilities::split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});
}

TEST_CASE("Utility Function Test: split(str, char, bool)")
{
  std::array<char, 1> k_Delimiter = {'|'};
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = StringUtilities::split(inputStr, k_Delimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|";

  result = StringUtilities::split(inputStr, k_Delimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  inputStr = "||This|Is||A||Baseline|Test||";

  result = StringUtilities::split(inputStr, k_Delimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 4
  inputStr = "This|Is|A|Baseline|Test";

  result = StringUtilities::split(inputStr, k_Delimiter, true);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 5
  inputStr = "|This|Is|A|Baseline|Test|";

  result = StringUtilities::split(inputStr, k_Delimiter, true);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "A", "Baseline", "Test", ""});

  // Case 6
  inputStr = "||This|Is||A||Baseline|Test||";

  result = StringUtilities::split(inputStr, k_Delimiter, true);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});
}

TEST_CASE("Utility Function Test: specific_split(str, char, StringUtilities::SplitType)")
{
  std::array<char, 1> k_Delimiter = {'|'};
  std::string inputStr = "||This|Is||A||Baseline|Test||";
  std::vector<std::string> result;

  // Case 1
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::AllowAll);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});

  // Case 2
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::IgnoreEmpty);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::NoStripIgnoreConsecutive);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "A", "Baseline", "Test", ""});

  // Case 4
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::OnlyConsecutive);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "", "A", "", "Baseline", "Test", ""});

  // Case 5
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::AllowEmptyLeftAnalyze);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", ""});

  // Case 5
  result = StringUtilities::specific_split(inputStr, k_Delimiter, StringUtilities::SplitType::AllowEmptyRightAnalyze);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});
}

#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;

TEST_CASE("StringInterpretationUtilities::Convert handles fractional Boolean text", "[simplnx][StringInterpretationUtilities]")
{
  const auto [input, expected] = GENERATE(table<std::string, bool>({
      {"0.5", true},
      {"-0.5", true},
      {"  +0.125  ", true},
      {"0.00001e-1", true},
      {".5", true},
      {"0.0", false},
      {"-0.0", false},
      {"0", false},
      {"-1", true},
      {"TRUE", true},
      {"False", false},
      {"0suffix", false},
      {"0.5suffix", false},
      {"1e-999", true},
      {"0e-999", false},
      {"not numeric", true},
  }));
  CAPTURE(input);
  const auto result = string_interpretation_utilities::Convert<bool>(input);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK(result.value() == expected);
}

TEST_CASE("Simplnx::StringUtilities::Split Test", "[Simplnx][StringUtilities]")
{
  std::string emptyInput;
  std::string noDelimiterInput = "ThisIsABaselineTest";

  std::vector<std::string> result;
  std::array<char, 1> kDelimiter = {'|'};

  // Case 1
  result = string_utilities::Split(emptyInput, kDelimiter[0]);

  REQUIRE(result.empty());

  result = string_utilities::Split(noDelimiterInput, kDelimiter[0]);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});

  // Case 2
  result = string_utilities::Split(emptyInput, kDelimiter, true);

  REQUIRE(result.empty());

  result = string_utilities::Split(noDelimiterInput, kDelimiter, true);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});

  // Case 4
  result = string_utilities::SpecificSplit(emptyInput, kDelimiter, string_utilities::SplitType::OnlyConsecutive);

  REQUIRE(result.empty());

  result = string_utilities::SpecificSplit(noDelimiterInput, kDelimiter, string_utilities::SplitType::OnlyConsecutive);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});

  // Case 5
  result = string_utilities::SpecificSplit(emptyInput, kDelimiter, string_utilities::SplitType::NoStripIgnoreConsecutive);

  REQUIRE(result.empty());

  result = string_utilities::SpecificSplit(noDelimiterInput, kDelimiter, string_utilities::SplitType::NoStripIgnoreConsecutive);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});

  // Case 6
  result = string_utilities::SpecificSplit(emptyInput, kDelimiter, string_utilities::SplitType::AllowEmptyLeftAnalyze);

  REQUIRE(result.empty());

  result = string_utilities::SpecificSplit(noDelimiterInput, kDelimiter, string_utilities::SplitType::AllowEmptyLeftAnalyze);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});

  // Case 7
  result = string_utilities::SpecificSplit(emptyInput, kDelimiter, string_utilities::SplitType::AllowEmptyRightAnalyze);

  REQUIRE(result.empty());

  result = string_utilities::SpecificSplit(noDelimiterInput, kDelimiter, string_utilities::SplitType::AllowEmptyRightAnalyze);

  REQUIRE(result == std::vector<std::string>{noDelimiterInput});
}

TEST_CASE("Simplnx::StringUtilities Utility Function Test1", "[Simplnx][StringUtilities]")
{
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = string_utilities::Split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|";

  result = string_utilities::Split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  inputStr = "||This|Is||A||Baseline|Test||";

  result = string_utilities::Split(inputStr, '|');

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});
}

TEST_CASE("Simplnx::StringUtilities Utility Function Test2", "[Simplnx][StringUtilities]")
{
  std::array<char, 1> kDelimiter = {'|'};
  // Case 1
  std::string inputStr = "This|Is|A|Baseline|Test";

  std::vector<std::string> result = string_utilities::Split(inputStr, kDelimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 2
  inputStr = "|This|Is|A|Baseline|Test|";

  result = string_utilities::Split(inputStr, kDelimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  inputStr = "||This|Is||A||Baseline|Test||";

  result = string_utilities::Split(inputStr, kDelimiter, false);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 4
  inputStr = "This|Is|A|Baseline|Test";

  result = string_utilities::Split(inputStr, kDelimiter, true);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 5
  inputStr = "|This|Is|A|Baseline|Test|";

  result = string_utilities::Split(inputStr, kDelimiter, true);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "A", "Baseline", "Test", ""});

  // Case 6
  inputStr = "||This|Is||A||Baseline|Test||";

  result = string_utilities::Split(inputStr, kDelimiter, true);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});
}

TEST_CASE("Simplnx::StringUtilities Utility Function Test3", "[Simplnx][StringUtilities]")
{
  std::array<char, 1> kDelimiter = {'|'};
  std::string inputStr = "||This|Is||A||Baseline|Test||";
  std::vector<std::string> result;

  // Case 1
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::AllowAll);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});

  // Case 2
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::IgnoreEmpty);

  REQUIRE(result == std::vector<std::string>{"This", "Is", "A", "Baseline", "Test"});

  // Case 3
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::NoStripIgnoreConsecutive);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "A", "Baseline", "Test", ""});

  // Case 4
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::OnlyConsecutive);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "", "A", "", "Baseline", "Test", ""});

  // Case 5
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::AllowEmptyLeftAnalyze);

  REQUIRE(result == std::vector<std::string>{"", "", "This", "Is", "", "A", "", "Baseline", "Test", ""});

  // Case 5
  result = string_utilities::SpecificSplit(inputStr, kDelimiter, string_utilities::SplitType::AllowEmptyRightAnalyze);

  REQUIRE(result == std::vector<std::string>{"", "This", "Is", "", "A", "", "Baseline", "Test", "", ""});
}

TEST_CASE("Simplnx::StringUtilities::Split preserves keep-empty contract", "[Simplnx][StringUtilities]")
{
  const std::array<char, 1> delimiters = {' '};
  REQUIRE(string_utilities::Split("1  2   3", delimiters, true) == std::vector<std::string>{"1", "", "2", "", "", "3"});
  REQUIRE(string_utilities::Split("1  2   3", delimiters, false) == std::vector<std::string>{"1", "2", "3"});
}

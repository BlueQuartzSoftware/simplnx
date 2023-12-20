#include "TestOne/Filters/ErrorWarningFilter.hpp"
#include "TestOne/TestOne_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
constexpr int32 k_PreflightWarning = -666000;
constexpr int32 k_PreflightError = -666001;
constexpr int32 k_ExecuteWarning = -666000;
constexpr int32 k_ExecuteError = -666001;
} // namespace

TEST_CASE("ErrorWarningFilter: Instantiate Filter", "[ErrorWarningFilter]")
{
  DataStructure dataStructure;

  // All booleans disabled
  {
    ErrorWarningFilter filter;

    // Create default Parameters for the filter.
    Arguments args;
    args.insertOrAssign(ErrorWarningFilter::k_PreflightWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_PreflightError_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteError_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }

  // Preflight Warning enabled
  {
    ErrorWarningFilter filter;

    // Create default Parameters for the filter.
    Arguments args;
    args.insertOrAssign(ErrorWarningFilter::k_PreflightWarning_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErrorWarningFilter::k_PreflightError_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteError_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    const std::vector<Warning>& warnings = preflightResult.outputActions.warnings();
    REQUIRE(warnings.size() == 1);
    REQUIRE(warnings[0].code == k_PreflightWarning);
  }

  // Preflight Error enabled
  {
    ErrorWarningFilter filter;

    // Create default Parameters for the filter.
    Arguments args;
    args.insertOrAssign(ErrorWarningFilter::k_PreflightWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_PreflightError_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteError_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

    const std::vector<Error>& errors = preflightResult.outputActions.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == k_PreflightError);
  }

  // Execute Warning enabled
  {
    ErrorWarningFilter filter;

    // Create default Parameters for the filter.
    Arguments args;
    args.insertOrAssign(ErrorWarningFilter::k_PreflightWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_PreflightError_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteWarning_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteError_Key, std::make_any<bool>(false));

    // Execute the filter and check result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<Warning>& warnings = executeResult.result.warnings();
    REQUIRE(warnings.size() == 1);
    REQUIRE(warnings[0].code == k_ExecuteWarning);
  }

  // Execute Error enabled
  {
    ErrorWarningFilter filter;

    // Create default Parameters for the filter.
    Arguments args;
    args.insertOrAssign(ErrorWarningFilter::k_PreflightWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_PreflightError_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteWarning_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErrorWarningFilter::k_ExecuteError_Key, std::make_any<bool>(true));

    // Execute the filter and check result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

    const std::vector<Error>& errors = executeResult.result.errors();
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].code == k_ExecuteError);
  }
}

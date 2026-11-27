#include "simplnx/Utilities/ImageProcessing/ImageTransferResult.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <utility>

using namespace nx::core;

TEST_CASE("Image transfer warnings: keep earlier warnings in order through a failed transfer", "[simplnx][ImageTransferResult]")
{
  Result<> accumulated;
  for(int32 code = 1; code <= 64; ++code)
  {
    ImageProcessing::AccumulateTransferResult(accumulated, MakeWarningVoidResult(code, "Page completed"));
  }
  ImageProcessing::AccumulateTransferResult(accumulated, {});
  auto failure = MakeErrorResult(-1, "Transfer failed");
  failure.warnings().push_back({.code = 65, .message = "Final page warning"});
  ImageProcessing::AccumulateTransferResult(accumulated, std::move(failure));
  // NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 comparison decomposition and result assertion.
  SIMPLNX_RESULT_REQUIRE_INVALID(accumulated);
  REQUIRE(accumulated.errors().size() == 1);
  REQUIRE(accumulated.errors().front().code == -1);
  REQUIRE(accumulated.warnings().size() == 65);
  for(usize i = 0; i < accumulated.warnings().size(); ++i)
  {
    REQUIRE(accumulated.warnings().at(i).code == static_cast<int32>(i + 1));
  }
  // NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
}

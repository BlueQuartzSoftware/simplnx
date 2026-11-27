#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ConnectedComponentEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <string>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros.
TEST_CASE("Connected component engine: native provisional label capacity", "[simplnx][ConnectedComponentEngine]")
{
  if constexpr(std::numeric_limits<usize>::max() <= std::numeric_limits<uint32>::max())
  {
    SUCCEED("A native index on this platform cannot exceed uint32 capacity; no overflowing candidate was executed.");
    return;
  }
  const std::array<uint64, 6> candidates{1, 4294967295ULL, 4294967296ULL, 4294967297ULL, 8589934592ULL, 8589934593ULL};
  const std::array<uint32, 6> legacyLabels{1, 4294967295U, 0, 1, 0, 1};
  usize warningMessages = 0;
  const IFilter::MessageHandler handler{[&warningMessages](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Warning)
    {
      ++warningMessages;
    }
  }};
  ImageProcessing::detail::ConnectedComponentLabelState strictState(ConnectedComponentLabelLimitHandling::Error, handler);
  ImageProcessing::detail::ConnectedComponentLabelState legacyState(ConnectedComponentLabelLimitHandling::WarnAndContinueLegacy, handler);
  for(usize index = 0; index < candidates.size(); ++index)
  {
    const usize candidate = static_cast<usize>(candidates.at(index));
    CAPTURE(candidate);
    const auto strictResult = strictState.labelFor(candidate);
    if(index < 2)
    {
      SIMPLNX_RESULT_REQUIRE_VALID(strictResult);
      REQUIRE(strictResult.value() == legacyLabels.at(index));
    }
    else
    {
      SIMPLNX_RESULT_REQUIRE_INVALID(strictResult);
      REQUIRE(strictResult.errors().front().code == -29301);
      REQUIRE(strictResult.errors().front().message.find(std::to_string(candidate)) != std::string::npos);
      REQUIRE(strictResult.errors().front().message.find("Warn and Continue (Legacy Behavior)") != std::string::npos);
    }
    const auto legacyResult = legacyState.labelFor(candidate);
    SIMPLNX_RESULT_REQUIRE_VALID(legacyResult);
    REQUIRE(legacyResult.value() == legacyLabels.at(index));
    REQUIRE(warningMessages == (index < 2 ? 0 : 1)); // immediate, before finish(), and only once
  }
  REQUIRE(warningMessages == 1);
  const auto strictFinished = strictState.finish({});
  SIMPLNX_RESULT_REQUIRE_VALID(strictFinished);
  REQUIRE(strictFinished.warnings().empty());
  const auto legacyFinished = legacyState.finish({});
  SIMPLNX_RESULT_REQUIRE_VALID(legacyFinished);
  REQUIRE(legacyFinished.warnings().size() == 1);
  REQUIRE(legacyFinished.warnings().front().code == -29302);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

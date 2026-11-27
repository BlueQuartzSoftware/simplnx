#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"

#include <catch2/catch.hpp>

#include <limits>

using namespace nx::core;

TEST_CASE("Image reconstruction: preserve native integer marker offsets", "[simplnx][MorphologicalReconstruction]")
{
  constexpr int64 intensity = int64{1} << 54;
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(ImageProcessing::detail::SubtractExactIntegerReconstructionHeight(intensity, 1) == intensity - 1);
  REQUIRE(ImageProcessing::detail::AddExactIntegerReconstructionHeight(intensity, 1) == intensity + 1);
  REQUIRE(ImageProcessing::detail::SubtractExactIntegerReconstructionHeight(std::numeric_limits<int64>::lowest(), 1) == std::numeric_limits<int64>::lowest());
  REQUIRE(ImageProcessing::detail::AddExactIntegerReconstructionHeight(std::numeric_limits<uint64>::max(), 1) == std::numeric_limits<uint64>::max());
  // NOLINTEND(bugprone-chained-comparison)
}

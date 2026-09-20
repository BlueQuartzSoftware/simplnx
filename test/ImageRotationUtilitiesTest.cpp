#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <string>

using namespace nx::core;

namespace
{
/**
 * @brief Gives the corner direction that FindOctant reports for one octant.
 * @param octant Specifies the octant index.
 * @return The sign of each axis, -1 toward the low side and 1 toward the high side.
 *
 * FindOctant returns the index of the nearest voxel corner. It builds the corners in the
 * order P1 to P8, so this table repeats that order. The test derives the expected offsets
 * from this order. It does not read them from the tables that it checks.
 */
std::array<int64, 3> CornerDirection(usize octant)
{
  // P1(-,-,-) P2(+,-,-) P3(+,+,-) P4(-,+,-) P5(-,-,+) P6(+,-,+) P7(+,+,+) P8(-,+,+)
  constexpr std::array<std::array<int64, 3>, 8> k_Directions = {{{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}};
  return k_Directions[octant];
}
} // namespace

TEST_CASE("ImageRotationUtilities: Octant offsets match the trilinear corner order", "[ImageRotationUtilities][Octant]")
{
  // calculateInterpolatedValue reads the eight offsets in this order:
  // [0]=c000 [1]=c100 [2]=c110 [3]=c010 [4]=c001 [5]=c101 [6]=c111 [7]=c011.
  // Each entry therefore selects a low or a high neighbor on each axis.
  constexpr std::array<std::array<int, 3>, 8> k_CornerIsHigh = {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}};

  for(usize octant = 0; octant < 8; ++octant)
  {
    const std::array<int64, 3> direction = CornerDirection(octant);
    // The cell spans from the source voxel toward the nearest corner.
    std::array<int64, 3> low{};
    std::array<int64, 3> high{};
    for(usize axis = 0; axis < 3; ++axis)
    {
      low[axis] = direction[axis] < 0 ? -1 : 0;
      high[axis] = direction[axis] < 0 ? 0 : 1;
    }

    const auto& offsets = ImageRotationUtilities::k_AllOctantOffsets[octant];
    for(usize corner = 0; corner < 8; ++corner)
    {
      for(usize axis = 0; axis < 3; ++axis)
      {
        const int64 expected = k_CornerIsHigh[corner][axis] == 1 ? high[axis] : low[axis];
        const std::string message = "octant " + std::to_string(octant) + " corner " + std::to_string(corner) + " axis " + std::to_string(axis);
        INFO(message);
        REQUIRE(offsets[corner][static_cast<int>(axis)] == expected);
      }
    }
  }
}

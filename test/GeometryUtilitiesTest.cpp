#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include <catch2/catch.hpp>
#include <limits>
using namespace nx::core;
TEST_CASE("GeometryUtilities: Finite vector domains", "[simplnx][GeometryUtilities]")
{
  const auto axis = GENERATE(0, 1, 2);
  const auto value = GENERATE(0.0f, -1.0f, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity());
  std::vector<float32> values = {1, 1, 1};
  values[axis] = value;
  auto positive = GeometryUtilities::ValidatePositiveFiniteVector(values, "Spacing", -1);
  SIMPLNX_RESULT_REQUIRE_INVALID(positive);
  CHECK(positive.errors()[0].code == -1);
  CHECK(positive.errors()[0].message.find("XYZ"[axis]) != std::string::npos);
  auto finite = GeometryUtilities::ValidateFiniteVector(values, "Origin", -2);
  if(std::isfinite(value))
  {
    SIMPLNX_RESULT_REQUIRE_VALID(finite);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(finite);
    CHECK(finite.errors()[0].code == -2);
    CHECK(finite.errors()[0].message.find("XYZ"[axis]) != std::string::npos);
  }
}
TEST_CASE("GeometryUtilities: Checked padded dimensions", "[simplnx][GeometryUtilities]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5);
  SizeVec3 dims = {1, 1, 1};
  std::array<std::array<int32, 2>, 3> padding = {{{0, 0}, {0, 0}, {0, 0}}};
  std::array<bool, 3> active = {true, true, true};
  const int32 max = std::numeric_limits<int32>::max();
  if(row == 0)
  {
    padding[0] = {max, max};
    active = {true, false, false};
  }
  if(row == 1)
  {
    dims[0] = std::numeric_limits<usize>::max();
    padding[0] = {1, 0};
  }
  if(row == 2)
  {
    padding = {{{max, max}, {max, max}, {max, max}}};
  }
  if(row == 3)
  {
    padding = {{{1, 2}, {3, 4}, {5, 6}}};
    dims = {2, 3, 4};
  }
  if(row == 4)
  {
    padding[1] = {-1, -1};
    active = {true, false, true};
  }
  auto result = GeometryUtilities::ComputePaddedDimensions(dims, padding, active);
  if(row == 1 || row == 2)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    CHECK(result.errors()[0].code == (row == 1 ? -86025 : -86026));
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.value() == (row == 0 ? SizeVec3{4294967295ULL, 1, 1} : row == 3 ? SizeVec3{5, 10, 15} : SizeVec3{1, 1, 1}));
  }
}

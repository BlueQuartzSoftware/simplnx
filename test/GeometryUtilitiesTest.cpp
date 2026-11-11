#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
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
TEST_CASE("GeometryTransformation: Selected vector validation", "[simplnx][GeometryTransformation]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
  DataStructure ds;
  REQUIRE(ImageGeom::Create(ds, "Image") != nullptr);
  REQUIRE(VertexGeom::Create(ds, "Vertices") != nullptr);
  ApplyTransformationToGeometryInputValues input{};
  input.SelectedGeometryPath = DataPath({row == 6 || row == 7 ? "Vertices" : "Image"});
  input.TransformationSelection = row < 3 ? 4 : row < 8 ? 5 : row < 11 ? 3 : 0;
  input.Translation = row == 0 ? std::vector<float32>{1, 2} : row == 1 ? std::vector<float32>{std::numeric_limits<float32>::infinity(), 0, 0} : std::vector<float32>{1, 2, 3};
  input.Scale = row == 3             ? std::vector<float32>{1, 2} :
                row == 4             ? std::vector<float32>{0, 1, 1} :
                row == 5 || row == 6 ? std::vector<float32>{std::numeric_limits<float32>::quiet_NaN(), 1, 1} :
                                       std::vector<float32>{-1, 0, 1};
  input.Rotation = row == 8 ? std::vector<float32>{0, 0, 1} : row == 9 ? std::vector<float32>{0, 0, 0, 90} : std::vector<float32>{0, 0, 2, 90};
  if(input.TransformationSelection != 4)
  {
    input.Translation.clear();
  }
  if(input.TransformationSelection != 5)
  {
    input.Scale.clear();
  }
  if(input.TransformationSelection != 3)
  {
    input.Rotation.clear();
  }
  if(row == 12)
  {
    input.TransformationSelection = 1;
  }
  if(row == 13)
  {
    input.TransformationSelection = 2;
  }
  if(row == 14)
  {
    input.TransformationSelection = 5;
    input.Scale = {1, 2, 3};
  }
  auto result = ValidateTransformationParameters(ds, input);
  const std::array<int32, 15> codes = {-86000, -86002, 0, -86001, -86004, -86004, -86003, 0, -86050, -86052, 0, 0, 0, 0, 0};
  if(codes[row] == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors()[0].code == codes[row]);
  }
}

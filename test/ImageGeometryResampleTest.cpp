#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <cmath>
#include <limits>

using namespace nx::core;

TEST_CASE("simplnx::ImageGeometryResample: Active parameter domains", "[simplnx][ImageGeometryResample]")
{
  ResampleImageGeomInputValues input{};
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  CAPTURE(row);
  int32 expectedCode = 0;
  std::string context;
  input.ResamplingMode = row < 4 ? 0 : 1;
  if(row < 4)
  {
    input.Spacing = {1, 1, 1};
    if(row == 0)
    {
      input.Spacing[0] = std::numeric_limits<float32>::denorm_min();
    }
    if(row == 1)
    {
      input.Spacing[0] = 0;
      expectedCode = -86113;
      context = "X value 0";
    }
    if(row == 2)
    {
      input.Spacing[1] = std::numeric_limits<float32>::quiet_NaN();
      expectedCode = -86113;
      context = "Y value nan";
    }
    if(row == 3)
    {
      input.Spacing[2] = -1;
      expectedCode = -11502;
      context = "Z value -1";
    }
  }
  else
  {
    input.Scaling = {100, 100, 100};
    if(row == 4)
    {
      input.Scaling[0] = std::numeric_limits<float32>::denorm_min();
    }
    if(row == 5)
    {
      input.Scaling[2] = 0;
      expectedCode = -11500;
      context = "Z value 0";
    }
    if(row == 6)
    {
      input.Scaling[0] = std::numeric_limits<float32>::quiet_NaN();
      expectedCode = -86114;
      context = "X value nan";
    }
    if(row == 7)
    {
      input.Spacing = {std::numeric_limits<float32>::quiet_NaN(), 0, -1};
      input.ExactDimensions = {0, 0, 0};
    }
    if(row == 8)
    {
      input.ResamplingMode = 2;
      input.ExactDimensions = {1, 2, 3};
      input.Spacing = {std::numeric_limits<float32>::quiet_NaN(), 0, -1};
      input.Scaling = {0, std::numeric_limits<float32>::quiet_NaN(), -std::numeric_limits<float32>::infinity()};
    }
  }
  if(row == 9)
  {
    input.ResamplingMode = 2;
    input.ExactDimensions = {std::numeric_limits<uint64>::max(), 1, 1};
  }
  const auto spacing = input.Spacing;
  const auto scaling = input.Scaling;
  const auto exact = input.ExactDimensions;
  auto result = ValidateResamplingParameters(input);
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.warnings().empty());
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == expectedCode);
    CHECK(result.errors()[0].message.find(row < 4 ? "Spacing" : "Scaling Factor") != std::string::npos);
    CHECK(result.errors()[0].message.find(context) != std::string::npos);
  }
  CHECK(input.ExactDimensions == exact);
  for(usize axis = 0; axis < spacing.size(); ++axis)
  {
    CHECK((input.Spacing[axis] == spacing[axis] || (std::isnan(input.Spacing[axis]) && std::isnan(spacing[axis]))));
  }
  for(usize axis = 0; axis < scaling.size(); ++axis)
  {
    CHECK((input.Scaling[axis] == scaling[axis] || (std::isnan(input.Scaling[axis]) && std::isnan(scaling[axis]))));
  }
}

TEST_CASE("simplnx::ImageGeometryResample: Checked layout arithmetic", "[simplnx][ImageGeometryResample]")
{
  DataStructure ds;
  auto* source = ImageGeom::Create(ds, "Source");
  REQUIRE(source != nullptr);
  source->setDimensions({4, 3, 2});
  source->setSpacing({2, 3, 4});
  source->setOrigin({-1, 0, 1});
  ResampleImageGeomInputValues input{};
  input.SelectedImageGeometryPath = DataPath({"Source"});
  int32 expectedCode = 0;
  SizeVec3 expectedDimensions = {2, 3, 4};
  FloatVec3 expectedSpacing = {4, 3, 2};
  std::string context;
  SECTION("Spacing ordinary")
  {
    input.Spacing = {4, 3, 2};
  }
  SECTION("Scaling ordinary")
  {
    input.ResamplingMode = 1;
    input.Scaling = {200, 50, 100};
    expectedDimensions = {8, 1, 2};
    expectedSpacing = {1, 6, 4};
  }
  SECTION("Exact ordinary")
  {
    input.ResamplingMode = 2;
    input.ExactDimensions = {2, 3, 4};
  }
  SECTION("Minimum one")
  {
    input.Spacing = {16, 18, 16};
    expectedDimensions = {1, 1, 1};
    expectedSpacing = {16, 18, 16};
  }
  SECTION("Large representable")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({1, 1, 1});
    input.Spacing = {std::ldexp(1.0F, -32), 1, 1};
    expectedDimensions = {usize{1} << 32, 1, 1};
    expectedSpacing = {input.Spacing[0], 1, 1};
  }
  SECTION("Spacing overflow")
  {
    source->setDimensions({2, 1, 1});
    source->setSpacing({3e38F, 1, 1});
    input.ResamplingMode = 2;
    input.ExactDimensions = {1, 1, 1};
    expectedCode = -86126;
    context = "derived Spacing X result inf";
  }
  SECTION("Spacing underflow")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({std::numeric_limits<float32>::denorm_min(), 1, 1});
    input.ResamplingMode = 1;
    input.Scaling = {200, 100, 100};
    expectedCode = -86126;
    context = "derived Spacing X result 0";
  }
  SECTION("Dimension exclusive bound")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({1, 1, 1});
    input.Spacing = {std::ldexp(1.0F, -64), 1, 1};
    expectedCode = -86127;
    context = "dimension conversion X";
  }
  SECTION("Tuple product")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({1, 1, 1});
    input.Spacing = {std::ldexp(1.0F, -32), std::ldexp(1.0F, -32), 1};
    expectedCode = -86128;
    context = "tuple product";
  }
  const auto originalDimensions = source->getDimensions();
  const auto originalSpacing = source->getSpacing();
  const auto originalOrigin = source->getOrigin();
  auto result = ComputeResampledImageLayout(*source, input);
  if(expectedCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    CHECK(result.warnings().empty());
    CHECK(result.value().Dimensions == expectedDimensions);
    CHECK(result.value().Spacing == expectedSpacing);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == expectedCode);
    CHECK(result.errors()[0].message.find(context) != std::string::npos);
    CHECK(result.errors()[0].message.find("Source") != std::string::npos);
  }
  CHECK(source->getDimensions() == originalDimensions);
  CHECK(source->getSpacing() == originalSpacing);
  CHECK(source->getOrigin() == originalOrigin);
}

TEST_CASE("simplnx::ImageGeometryResample: Axis conversion bound", "[simplnx][ImageGeometryResample]")
{
  auto result = ResampleImageGeom::ComputeAxisSourceIndices(1, std::ldexp(1.0, 64), 1, std::numeric_limits<usize>::max(), 0, 1);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -86137);
  CHECK(result.errors()[0].message.find("exclusive bound") != std::string::npos);
}

TEST_CASE("simplnx::ImageGeometryResample: Prepared metadata stops copy", "[simplnx][ImageGeometryResample]")
{
  DataStructure ds;
  for(const auto& name : {"Source", "Destination"})
  {
    auto geometry = CreateImageGeometryAction(DataPath({name}), {2, 1, 1}, {0, 0, 0}, {1, 1, 1}, "Cell Data").apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(geometry);
    auto array = CreateArrayAction(DataType::int32, {1, 1, 2}, {1}, DataPath({name, "Cell Data", "Marker"})).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(array);
    REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({name, "Cell Data", "Marker"})));
    auto& values = ds.getDataRefAs<Int32Array>(DataPath({name, "Cell Data", "Marker"}));
    values[0] = name == std::string("Source") ? 7 : 31;
    values[1] = name == std::string("Source") ? 9 : 37;
  }
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(DataPath({"Destination"})));
  ds.getDataRefAs<ImageGeom>(DataPath({"Destination"})).setSpacing({std::numeric_limits<float32>::quiet_NaN(), 1, 1});
  ResampleImageGeomInputValues input{};
  input.SelectedImageGeometryPath = DataPath({"Source"});
  input.CreatedImageGeometryPath = DataPath({"Destination"});
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  auto result = ResampleImageGeom(ds, handler, cancel, &input)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -86136);
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(DataPath({"Destination", "Cell Data", "Marker"})));
  const auto& values = ds.getDataRefAs<Int32Array>(DataPath({"Destination", "Cell Data", "Marker"}));
  CHECK(values[0] == 31);
  CHECK(values[1] == 37);
}

TEST_CASE("simplnx::PreflightImageGeometryResample: the planned geometry keeps the source length unit", "[simplnx][PreflightImageGeometryResample]")
{
  const auto units = GENERATE(IGeometry::LengthUnit::Millimeter, IGeometry::LengthUnit::Meter, IGeometry::LengthUnit::Unspecified);
  CAPTURE(units);

  DataStructure dataStructure;
  const DataPath srcGeomPath({"Input Geometry"});
  const CreateImageGeometryAction inputGeometryAction(srcGeomPath, {2, 2, 2}, {1.0F, -2.0F, 3.0F}, {1.0F, 1.0F, 1.0F}, "Cell Data", units);
  auto inputGeometryResult = inputGeometryAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(inputGeometryResult);

  ResampleImageGeomInputValues inputValues;
  inputValues.ResamplingMode = 0;
  inputValues.Spacing = {0.5F, 0.5F, 0.5F};
  inputValues.SelectedImageGeometryPath = srcGeomPath;
  inputValues.RemoveOriginalImageGeom = false;
  inputValues.CreatedImageGeometryPath = DataPath({"Resampled Geometry"});
  inputValues.RenumberFeatures = false;

  const auto preflightResult = PreflightImageGeometryResample(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const CreateImageGeometryAction* plannedGeometryPtr = nullptr;
  for(const auto& action : preflightResult.outputActions.value().actions)
  {
    if(const auto* candidatePtr = dynamic_cast<const CreateImageGeometryAction*>(action.get()); candidatePtr != nullptr)
    {
      plannedGeometryPtr = candidatePtr;
      break;
    }
  }
  REQUIRE(plannedGeometryPtr != nullptr);
  CHECK(plannedGeometryPtr->units() == units);
  CHECK(plannedGeometryPtr->path() == inputValues.CreatedImageGeometryPath);
}

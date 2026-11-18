#include "simplnx/Utilities/ImageProcessing/ImageGeometryResample.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>

using namespace nx::core;

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
    context = "invalid New Spacing on the X axis (inf)";
  }
  SECTION("Spacing underflow")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({std::numeric_limits<float32>::denorm_min(), 1, 1});
    input.ResamplingMode = 1;
    input.Scaling = {200, 100, 100};
    expectedCode = -86126;
    context = "invalid New Spacing on the X axis (0)";
  }
  SECTION("Dimension exclusive bound")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({1, 1, 1});
    input.Spacing = {std::ldexp(1.0F, -64), 1, 1};
    expectedCode = -86127;
    context = "calculated cell count on the X axis";
  }
  SECTION("Tuple product")
  {
    source->setDimensions({1, 1, 1});
    source->setSpacing({1, 1, 1});
    input.Spacing = {std::ldexp(1.0F, -32), std::ldexp(1.0F, -32), 1};
    expectedCode = -86128;
    context = "too many cells";
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
  CHECK(result.errors()[0].message.find("cannot find a source cell") != std::string::npos);
}

namespace
{
constexpr int32 k_InjectionError = -91014;

class FailingResampleSourceStore : public DataStore<int32>
{
public:
  explicit FailingResampleSourceStore(std::promise<void>& readStarted)
  : DataStore<int32>({1, 1, 2}, {1}, 0)
  , m_ReadStarted(readStarted)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<int32>) const override
  {
    m_ReadStarted.set_value();
    return MakeErrorResult(k_InjectionError, "Injected source read failure");
  }

private:
  std::promise<void>& m_ReadStarted;
};
} // namespace

TEST_CASE("simplnx::ImageGeometryResample: Cancellation preserves a source read error", "[simplnx][ImageGeometryResample]")
{
  DataStructure ds;
  std::promise<void> readStarted;
  auto firstRead = readStarted.get_future();
  for(const auto& name : {"Source", "Destination"})
  {
    auto result = CreateImageGeometryAction(DataPath({name}), {2, 1, 1}, {0, 0, 0}, {1, 1, 1}, "Cell Data").apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(const auto& arrayName : {"First", "Second", "Third"})
    {
      const DataPath path({name, "Cell Data", arrayName});
      result = CreateArrayAction(DataType::int32, {1, 1, 2}, {1}, path).apply(ds, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      if(std::string_view(name) == "Source")
      {
        auto& array = ds.getDataRefAs<Int32Array>(path);
        result = array.setDataStore(std::make_shared<FailingResampleSourceStore>(readStarted));
        SIMPLNX_RESULT_REQUIRE_VALID(result);
      }
    }
  }
  std::atomic_bool cancel = false;
  int32 announcements = 0;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Info && ++announcements == 2)
    {
      if(firstRead.wait_for(std::chrono::seconds(5)) == std::future_status::ready)
      {
        cancel = true;
      }
    }
  }};
  ResampleImageGeomInputValues input{};
  input.SelectedImageGeometryPath = DataPath({"Source"});
  input.CreatedImageGeometryPath = DataPath({"Destination"});
  auto result = ResampleImageGeom(ds, handler, cancel, &input)();
  REQUIRE(firstRead.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  REQUIRE(cancel.load());
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == k_InjectionError);
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
  auto& destination = ds.getDataRefAs<ImageGeom>(DataPath({"Destination"}));
  int32 expectedCode = -86136;
  SECTION("Spacing")
  {
    destination.setSpacing({std::numeric_limits<float32>::quiet_NaN(), 1, 1});
  }
  SECTION("Origin")
  {
    destination.setOrigin({std::numeric_limits<float32>::infinity(), 0, 0});
    expectedCode = -86228;
  }
  SECTION("Cell count")
  {
    destination.setDimensions({usize{1} << 63, 2, 1});
    expectedCode = -86229;
  }
  SECTION("Row count")
  {
    destination.setDimensions({0, usize{1} << 63, 2});
    expectedCode = -86230;
  }
  SECTION("Rows to copy")
  {
    auto array = CreateArrayAction(DataType::int32, {1, 1, 2}, {1}, DataPath({"Destination", "Cell Data", "Extra"})).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(array);
    destination.setDimensions({1, usize{1} << 63, 1});
    expectedCode = -86231;
  }
  SECTION("Array value count")
  {
    auto array = CreateArrayAction(DataType::int32, {1, 1, 2}, {16}, DataPath({"Destination", "Cell Data", "Extra"})).apply(ds, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(array);
    const auto* extra = ds.getDataAs<Int32Array>(DataPath({"Destination", "Cell Data", "Extra"}));
    REQUIRE(extra != nullptr);
    REQUIRE(extra->getNumberOfComponents() == 16);
    destination.setDimensions({usize{1} << 60, 1, 1});
    expectedCode = -86232;
  }
  SECTION("Array byte count")
  {
    destination.setDimensions({usize{1} << 62, 1, 1});
    expectedCode = -86233;
  }
  SECTION("Axis length")
  {
    destination.setDimensions({std::vector<usize>{}.max_size() + 1, 1, 1});
    expectedCode = -86234;
  }
  ResampleImageGeomInputValues input{};
  input.SelectedImageGeometryPath = DataPath({"Source"});
  input.CreatedImageGeometryPath = DataPath({"Destination"});
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  auto result = ResampleImageGeom(ds, handler, cancel, &input)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CAPTURE(expectedCode);
  CHECK(result.errors()[0].code == expectedCode);
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

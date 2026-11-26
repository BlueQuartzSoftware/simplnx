#include "SimplnxCore/Filters/AlignGeometriesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
DataStructure CreateTestData()
{
  DataStructure dataStructure;
  auto* movingGeomPtr = ImageGeom::Create(dataStructure, "Moving Geometry");
  auto* targetGeomPtr = ImageGeom::Create(dataStructure, "Target Geometry");

  SizeVec3 dimensions(5, 10, 15);
  FloatVec3 origin1(0, 0, 0);
  FloatVec3 origin2(50, 100, 60);

  movingGeomPtr->setDimensions(dimensions);
  targetGeomPtr->setDimensions(dimensions);
  REQUIRE(movingGeomPtr->setOrigin(origin1).valid());
  REQUIRE(targetGeomPtr->setOrigin(origin2).valid());

  return dataStructure;
}

RectGridGeom& CreateRectGrid(DataStructure& dataStructure, const std::string& name, const std::vector<float32>& xb, const std::vector<float32>& yb, const std::vector<float32>& zb)
{
  REQUIRE(xb.size() >= 2);
  REQUIRE(yb.size() >= 2);
  REQUIRE(zb.size() >= 2);
  auto* rectGridGeomPtr = RectGridGeom::Create(dataStructure, name);
  REQUIRE(rectGridGeomPtr != nullptr);
  rectGridGeomPtr->setDimensions(SizeVec3(xb.size() - 1, yb.size() - 1, zb.size() - 1));

  auto* xBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "xBounds", ShapeType{xb.size()}, ShapeType{1}, rectGridGeomPtr->getId());
  auto* yBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "yBounds", ShapeType{yb.size()}, ShapeType{1}, rectGridGeomPtr->getId());
  auto* zBoundsPtr = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "zBounds", ShapeType{zb.size()}, ShapeType{1}, rectGridGeomPtr->getId());
  REQUIRE(xBoundsPtr != nullptr);
  REQUIRE(yBoundsPtr != nullptr);
  REQUIRE(zBoundsPtr != nullptr);
  for(usize boundIdx = 0; boundIdx < xb.size(); boundIdx++)
  {
    xBoundsPtr->setValue(boundIdx, xb[boundIdx]);
  }
  for(usize boundIdx = 0; boundIdx < yb.size(); boundIdx++)
  {
    yBoundsPtr->setValue(boundIdx, yb[boundIdx]);
  }
  for(usize boundIdx = 0; boundIdx < zb.size(); boundIdx++)
  {
    zBoundsPtr->setValue(boundIdx, zb[boundIdx]);
  }
  const auto boundsResult = rectGridGeomPtr->setBounds(xBoundsPtr, yBoundsPtr, zBoundsPtr);
  SIMPLNX_RESULT_REQUIRE_VALID(boundsResult);
  return *rectGridGeomPtr;
}
} // namespace

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Instantiate Filter", "[AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  AlignGeometriesFilter filter;
  DataStructure dataStructure = CreateTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Invalid"});
  DataPath targetGeomPath = DataPath({"Invalid"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Bad Alignment Type", "[AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  AlignGeometriesFilter filter;
  DataStructure dataStructure = CreateTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 3;

  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Valid Arguments", "[AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  AlignGeometriesFilter filter;
  DataStructure dataStructure = CreateTestData();
  Arguments args;

  DataPath movingGeomPath = DataPath({"Moving Geometry"});
  DataPath targetGeomPath = DataPath({"Target Geometry"});
  uint64 alignmentType = 0;

  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(movingGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(targetGeomPath));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<uint64>(alignmentType));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& movingGeom = dataStructure.getDataRefAs<ImageGeom>(movingGeomPath);
  auto& targetGeom = dataStructure.getDataRefAs<ImageGeom>(targetGeomPath);

  REQUIRE(movingGeom.getOrigin() == targetGeom.getOrigin());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Centroid Image", "[SimplnxCore][AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* movingGeomPtr = ImageGeom::Create(dataStructure, "Moving Geometry");
  auto* targetGeomPtr = ImageGeom::Create(dataStructure, "Target Geometry");
  REQUIRE(movingGeomPtr != nullptr);
  REQUIRE(targetGeomPtr != nullptr);
  movingGeomPtr->setDimensions(SizeVec3(2, 4, 6));
  REQUIRE(movingGeomPtr->setSpacing(FloatVec3(0.5f, 1.0f, 2.0f)).valid());
  REQUIRE(movingGeomPtr->setOrigin(FloatVec3(-1.0f, -2.0f, -3.0f)).valid());
  targetGeomPtr->setDimensions(SizeVec3(4, 2, 3));
  REQUIRE(targetGeomPtr->setSpacing(FloatVec3(1.0f, 1.0f, 1.0f)).valid());
  REQUIRE(targetGeomPtr->setOrigin(FloatVec3(10.0f, 20.0f, 30.0f)).valid());

  AlignGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(DataPath({"Moving Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(DataPath({"Target Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The centroids are origin + dimensions * spacing / 2: (-0.5, 0, 3) and (12, 21, 31.5).
  // The translation is (12.5, 21, 28.5). Add it to the moving origin (-1, -2, -3).
  REQUIRE(movingGeomPtr->getOrigin() == FloatVec3(11.5f, 19.0f, 25.5f));
  REQUIRE(targetGeomPtr->getOrigin() == FloatVec3(10.0f, 20.0f, 30.0f));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Centroid RectGrid Target", "[SimplnxCore][AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* movingGeomPtr = ImageGeom::Create(dataStructure, "Moving Geometry");
  REQUIRE(movingGeomPtr != nullptr);
  movingGeomPtr->setDimensions(SizeVec3(2, 4, 6));
  REQUIRE(movingGeomPtr->setSpacing(FloatVec3(1.0f, 1.0f, 1.0f)).valid());
  REQUIRE(movingGeomPtr->setOrigin(FloatVec3(0.0f, 0.0f, 0.0f)).valid());
  CreateRectGrid(dataStructure, "Target Geometry", {10.0f, 11.0f, 13.0f}, {-6.0f, -5.0f, -2.0f}, {4.0f, 8.0f});

  AlignGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(DataPath({"Moving Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(DataPath({"Target Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<ChoicesParameter::ValueType>(1));
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The target centroid is ((10 + 13) / 2, (-6 - 2) / 2, (4 + 8) / 2) = (11.5, -4, 6).
  // Subtract the moving centroid (1, 2, 3) to get the translation (10.5, -6, 3).
  REQUIRE(movingGeomPtr->getOrigin() == FloatVec3(10.5f, -6.0f, 3.0f));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Origin RectGrid Moving", "[SimplnxCore][AlignGeometriesFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto& movingGeom = CreateRectGrid(dataStructure, "Moving Geometry", {0.0f, 1.0f, 3.0f}, {0.0f, 2.0f}, {0.0f, 5.0f, 6.0f, 9.0f});
  auto* targetGeomPtr = ImageGeom::Create(dataStructure, "Target Geometry");
  REQUIRE(targetGeomPtr != nullptr);
  targetGeomPtr->setDimensions(SizeVec3(1, 1, 1));
  REQUIRE(targetGeomPtr->setSpacing(FloatVec3(1.0f, 1.0f, 1.0f)).valid());
  REQUIRE(targetGeomPtr->setOrigin(FloatVec3(10.0f, -20.0f, 30.5f)).valid());

  AlignGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, std::make_any<DataPath>(DataPath({"Moving Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, std::make_any<DataPath>(DataPath({"Target Geometry"})));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // The moving origin is (0, 0, 0). Add the translation (10, -20, 30.5) to every bound.
  const std::vector<float32> expectedX = {10.0f, 11.0f, 13.0f};
  const std::vector<float32> expectedY = {-20.0f, -18.0f};
  const std::vector<float32> expectedZ = {30.5f, 35.5f, 36.5f, 39.5f};
  for(usize boundIdx = 0; boundIdx < expectedX.size(); boundIdx++)
  {
    CAPTURE(boundIdx);
    REQUIRE(movingGeom.getXBounds()->getDataStoreRef()[boundIdx] == expectedX[boundIdx]);
  }
  for(usize boundIdx = 0; boundIdx < expectedY.size(); boundIdx++)
  {
    CAPTURE(boundIdx);
    REQUIRE(movingGeom.getYBounds()->getDataStoreRef()[boundIdx] == expectedY[boundIdx]);
  }
  for(usize boundIdx = 0; boundIdx < expectedZ.size(); boundIdx++)
  {
    CAPTURE(boundIdx);
    REQUIRE(movingGeom.getZBounds()->getDataStoreRef()[boundIdx] == expectedZ[boundIdx]);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: SIMPL Backwards Compatibility", "[SimplnxCore][AlignGeometriesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "AlignGeometriesFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<AlignGeometriesFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(AlignGeometriesFilter::k_AlignmentType_Key) == 0);
      CHECK(args.value<DataPath>(AlignGeometriesFilter::k_MovingGeometry_Key) == DataPath({"MovingGeometry"}));
      CHECK(args.value<DataPath>(AlignGeometriesFilter::k_TargetGeometry_Key) == DataPath({"TargetGeometry"}));
    }
  }
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: Rejects collapsed translated RectGrid bounds", "[SimplnxCore][AlignGeometriesFilter]")
{
  DataStructure dataStructure;
  auto& moving = CreateRectGrid(dataStructure, "Moving Grid", {0.0F, 1.0F}, {0.0F, 0.001F}, {0.0F, 1.0F});
  auto* targetPtr = ImageGeom::Create(dataStructure, "Target Image");
  REQUIRE(targetPtr != nullptr);
  targetPtr->setDimensions({1, 1, 1});
  const auto origin = targetPtr->setOrigin({10.0F, 1000000.0F, 0.0F});
  SIMPLNX_RESULT_REQUIRE_VALID(origin);

  AlignGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, DataPath({"Moving Grid"}));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, DataPath({"Target Image"}));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, ChoicesParameter::ValueType{0});
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  CHECK(result.result.errors().front().code == -4007);
  CHECK(result.result.errors().front().message.find("Y bounds") != std::string::npos);
  CHECK(moving.getXBounds()->getValue(0) == 0.0F);
  CHECK(moving.getXBounds()->getValue(1) == 1.0F);
  CHECK(moving.getYBounds()->getValue(0) == 0.0F);
  CHECK(moving.getYBounds()->getValue(1) == 0.001F);
}

TEST_CASE("SimplnxCore::AlignGeometriesFilter: translates shared RectGrid bounds once", "[SimplnxCore][AlignGeometriesFilter]")
{
  const bool equalTranslations = GENERATE(true, false);
  CAPTURE(equalTranslations);
  DataStructure dataStructure;
  auto& moving = CreateRectGrid(dataStructure, "Moving Grid", {8388607.0F, 8388608.0F}, {0.0F, 1.0F}, {0.0F, 1.0F});
  auto* sharedBoundsPtr = moving.getXBounds();
  REQUIRE(moving.setBounds(sharedBoundsPtr, sharedBoundsPtr, moving.getZBounds()).valid());

  auto* targetPtr = ImageGeom::Create(dataStructure, "Target Image");
  REQUIRE(targetPtr != nullptr);
  targetPtr->setDimensions({1, 1, 1});
  REQUIRE(targetPtr->setOrigin({16777215.0F, equalTranslations ? 16777215.0F : 8388607.0F, 3.0F}).valid());

  AlignGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(AlignGeometriesFilter::k_MovingGeometry_Key, DataPath({"Moving Grid"}));
  args.insertOrAssign(AlignGeometriesFilter::k_TargetGeometry_Key, DataPath({"Target Image"}));
  args.insertOrAssign(AlignGeometriesFilter::k_AlignmentType_Key, ChoicesParameter::ValueType{0});
  const auto result = filter.execute(dataStructure, args);
  if(equalTranslations)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    CHECK(sharedBoundsPtr->getValue(0) == 16777215.0F);
    CHECK(sharedBoundsPtr->getValue(1) == 16777216.0F);
    CHECK(moving.getZBounds()->getValue(0) == 3.0F);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    CHECK(result.result.errors().front().code == -69201);
    CHECK(sharedBoundsPtr->getValue(0) == 8388607.0F);
    CHECK(sharedBoundsPtr->getValue(1) == 8388608.0F);
    CHECK(moving.getZBounds()->getValue(0) == 0.0F);
  }
}

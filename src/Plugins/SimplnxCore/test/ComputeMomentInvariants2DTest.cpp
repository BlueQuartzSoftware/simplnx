#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <any>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <string>
#include <vector>

#include "SimplnxCore/Filters/Algorithms/ComputeMomentInvariants2D.hpp"
#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/ComputeMomentInvariants2DFilter.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{

DataStructure CreateInvalidTestData()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometry);
  constexpr size_t dimsIn[3] = {5, 5, 5};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 5;
  dims[1] = 5;
  dims[2] = 5;
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellData, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIds, dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureData, dims, imageGeom->getId());

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif
  const std::vector<size_t> compDims(1, 6);
  UInt32Array* rect = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, k_RectCoords, dims, compDims, featureAm->getId());
  rect->fill(0);
  (*rect)[6] = 1;
  (*rect)[7] = 1;
  (*rect)[8] = 0;
  (*rect)[9] = 3;
  (*rect)[10] = 3;
  (*rect)[11] = 0;

  return dataStructure;
}

DataStructure CreateTestData()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometry);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;
  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellData, dims, imageGeom->getId());

  Int32Array* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_FeatureIds, dims, std::vector<usize>{1}, cellAm->getId());
  featureIds->fill(0);
  (*featureIds)[6] = 1;
  (*featureIds)[7] = 1;
  (*featureIds)[8] = 1;
  (*featureIds)[11] = 1;
  (*featureIds)[12] = 1;
  (*featureIds)[13] = 1;
  (*featureIds)[16] = 1;
  (*featureIds)[17] = 1;
  (*featureIds)[18] = 1;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureData, dims, imageGeom->getId());

#if 0
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0;
#endif
  const std::vector<size_t> compDims(1, 6);
  UInt32Array* rect = UInt32Array::CreateWithStore<DataStore<uint32>>(dataStructure, k_RectCoords, dims, compDims, featureAm->getId());
  rect->fill(0);
  (*rect)[6] = 1;
  (*rect)[7] = 1;
  (*rect)[8] = 0;
  (*rect)[9] = 3;
  (*rect)[10] = 3;
  (*rect)[11] = 0;

  return dataStructure;
}

const DataPath k_Omega1Path({k_ImageGeometry, k_FeatureData, k_Omega1});
const DataPath k_Omega2Path({k_ImageGeometry, k_FeatureData, k_Omega2});
const DataPath k_FeatureDataPath({k_ImageGeometry, k_FeatureData});
const DataPath k_FeatureIdsPath({k_ImageGeometry, k_CellData, k_FeatureIds});
const DataPath k_RectCoordsPath = k_FeatureDataPath.createChildPath(k_RectCoords);
const DataPath k_CentralMomentsPath = k_FeatureDataPath.createChildPath(k_CentralMoments);

DataStructure createUnusedFeatureTestData(int32 liveId)
{
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, k_ImageGeometry);
  REQUIRE((imageGeomPtr != nullptr));
  imageGeomPtr->setDimensions({5, 5, 1});
  imageGeomPtr->setOrigin({0, 0, 0});
  imageGeomPtr->setSpacing({1, 1, 1});
  imageGeomPtr->setUnits(IGeometry::LengthUnit::Micrometer);
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, k_CellData, ShapeType{1, 5, 5}, imageGeomPtr->getId());
  REQUIRE((cellAMPtr != nullptr));
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, k_FeatureData, ShapeType{65537}, imageGeomPtr->getId());
  REQUIRE((featureAMPtr != nullptr));
  auto featureIdsStorePtr = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, ShapeType{1, 5, 5}, ShapeType{1});
  REQUIRE((featureIdsStorePtr != nullptr));
  auto* featureIdsPtr = Int32Array::Create(dataStructure, k_FeatureIds, featureIdsStorePtr, cellAMPtr->getId());
  REQUIRE((featureIdsPtr != nullptr));
  featureIdsPtr->fill(0);
  for(usize row = 1; row <= 3; ++row)
  {
    for(usize column = 1; column <= 3; ++column)
    {
      (*featureIdsPtr)[(5 * row) + column] = liveId;
    }
  }

  const ComputeFeatureRectFilter cornerFilter;
  auto cornerArgs = cornerFilter.getDefaultArguments();
  cornerArgs.insertOrAssign(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  cornerArgs.insertOrAssign(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureDataPath));
  cornerArgs.insertOrAssign(ComputeFeatureRectFilter::k_FeatureRectArrayName_Key, std::make_any<std::string>(k_RectCoords));
  const auto cornerResult = cornerFilter.execute(dataStructure, cornerArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(cornerResult.result);

  const auto* rectPtr = dataStructure.getDataAs<UInt32Array>(k_RectCoordsPath);
  REQUIRE((rectPtr != nullptr));
  constexpr std::array<uint32, 6> expectedRect = {1, 1, 0, 3, 3, 0};
  for(usize component = 0; component < expectedRect.size(); ++component)
  {
    REQUIRE(((*rectPtr)[(static_cast<usize>(liveId) * 6) + component] == expectedRect.at(component)));
    if(liveId == 2)
    {
      const uint32 expectedUnused = component < 3 ? std::numeric_limits<uint32>::max() : 0U;
      REQUIRE(((*rectPtr)[6 + component] == expectedUnused));
    }
  }
  return dataStructure;
}

Arguments createUnusedFeatureArguments(bool saveCentralMoments)
{
  const ComputeMomentInvariants2DFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(k_RectCoordsPath));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureDataPath));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key, std::make_any<bool>(saveCentralMoments));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key, std::make_any<std::string>(k_Omega1));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key, std::make_any<std::string>(k_Omega2));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key, std::make_any<std::string>(k_CentralMoments));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: Unused feature regression 1775", "[SimplnxCore][ComputeMomentInvariants2DFilter][Issue1775]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE((appPtr != nullptr));
  const bool saveCentralMoments = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(saveCentralMoments, scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  std::array<float32, 2> referenceOmega{};
  std::array<float32, 9> referenceCentral{};
  constexpr std::array<int32, 2> liveIds = {1, 2};

  // A centered square of side 3 has area 9, mu20 = mu02 = 27/4, and mu22 = 81/16.
  constexpr float64 expectedOmega1 = 3.0 / numbers::pi;
  constexpr float64 expectedOmega2 = 9.0 / (numbers::pi * numbers::pi);
  constexpr std::array<float64, 9> expectedCentral = {9.0, 0.0, 6.75, 0.0, 0.0, 0.0, 6.75, 0.0, 5.0625};
  constexpr float64 tolerance = 0.00001;
  for(const int32 liveId : liveIds)
  {
    CAPTURE(liveId);
    DataStructure dataStructure = createUnusedFeatureTestData(liveId);
    const ComputeMomentInvariants2DFilter filter;
    const auto args = createUnusedFeatureArguments(saveCentralMoments);
    const auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const auto* omega1Ptr = dataStructure.getDataAs<Float32Array>(k_Omega1Path);
    const auto* omega2Ptr = dataStructure.getDataAs<Float32Array>(k_Omega2Path);
    REQUIRE((omega1Ptr != nullptr));
    REQUIRE((omega2Ptr != nullptr));
    scope.requireExpectedStore(*omega1Ptr);
    scope.requireExpectedStore(*omega2Ptr);
    const usize liveFeatureIndex = static_cast<usize>(liveId);
    CHECK((std::fabs(static_cast<float64>((*omega1Ptr)[liveFeatureIndex]) - expectedOmega1) < tolerance));
    CHECK((std::fabs(static_cast<float64>((*omega2Ptr)[liveFeatureIndex]) - expectedOmega2) < tolerance));
    CHECK(((*omega1Ptr)[0] == 0.0F));
    CHECK(((*omega2Ptr)[0] == 0.0F));
    CHECK(((*omega1Ptr)[65536] == 0.0F));
    CHECK(((*omega2Ptr)[65536] == 0.0F));
    if(liveId == 1)
    {
      referenceOmega = {(*omega1Ptr)[1], (*omega2Ptr)[1]};
    }
    else
    {
      CHECK(((*omega1Ptr)[2] == referenceOmega[0]));
      CHECK(((*omega2Ptr)[2] == referenceOmega[1]));
      CHECK(((*omega1Ptr)[1] == 0.0F));
      CHECK(((*omega2Ptr)[1] == 0.0F));
    }

    const auto* centralPtr = dataStructure.getDataAs<Float32Array>(k_CentralMomentsPath);
    if(saveCentralMoments)
    {
      REQUIRE((centralPtr != nullptr));
      scope.requireExpectedStore(*centralPtr);
      for(usize component = 0; component < expectedCentral.size(); ++component)
      {
        CAPTURE(component);
        const float32 value = (*centralPtr)[(liveFeatureIndex * 9) + component];
        CHECK((std::fabs(static_cast<float64>(value) - expectedCentral.at(component)) < tolerance));
        CHECK(((*centralPtr)[component] == 0.0F));
        CHECK(((*centralPtr)[(usize{65536} * 9) + component] == 0.0F));
        if(liveId == 1)
        {
          referenceCentral.at(component) = value;
        }
        else
        {
          CHECK((value == referenceCentral.at(component)));
          CHECK(((*centralPtr)[9 + component] == 0.0F));
        }
      }
    }
    else
    {
      CHECK((centralPtr == nullptr));
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

#ifndef SIMPLNX_ENABLE_MULTICORE
TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: Cancel after unused feature", "[SimplnxCore][ComputeMomentInvariants2DFilter][Issue1775]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE((appPtr != nullptr));
  const bool saveCentralMoments = GENERATE(false, true);
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(saveCentralMoments, scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure = createUnusedFeatureTestData(2);
  const ComputeMomentInvariants2DFilter filter;
  const auto args = createUnusedFeatureArguments(saveCentralMoments);
  std::atomic_bool shouldCancel = false;
  const IFilter::MessageHandler messageHandler{[&shouldCancel](const IFilter::Message& message) {
    if(message.message == "Feature 1 is NOT strictly 2D in the XY plane. Skipping this feature.")
    {
      shouldCancel = true;
    }
  }};

  // The first serial skip reports immediately, before the worker reaches live feature 2.
  const auto executeResult = scope.execute([&] { return filter.execute(dataStructure, args, nullptr, messageHandler, shouldCancel); });
  REQUIRE(shouldCancel.load());
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE((executeResult.result.errors().size() == 1));
  CHECK((executeResult.result.errors()[0].code == -1));
  CHECK((executeResult.result.errors()[0].message == "Filter cancelled"));

  const auto* omega1Ptr = dataStructure.getDataAs<Float32Array>(k_Omega1Path);
  const auto* omega2Ptr = dataStructure.getDataAs<Float32Array>(k_Omega2Path);
  REQUIRE((omega1Ptr != nullptr));
  REQUIRE((omega2Ptr != nullptr));
  CHECK(((*omega1Ptr)[2] == 0.0F));
  CHECK(((*omega2Ptr)[2] == 0.0F));
  if(saveCentralMoments)
  {
    const auto* centralPtr = dataStructure.getDataAs<Float32Array>(k_CentralMomentsPath);
    REQUIRE((centralPtr != nullptr));
    for(usize component = 0; component < 9; ++component)
    {
      CHECK(((*centralPtr)[18 + component] == 0.0F));
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
#endif

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: Valid Filter Execution", "[SimplnxCore][ComputeMomentInvariants2DFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ComputeMomentInvariants2DFilter filter;
  DataStructure ds = CreateTestData();
  Arguments args;

  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData, k_RectCoords})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key, std::make_any<std::string>(k_Omega1));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key, std::make_any<std::string>(k_Omega2));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key, std::make_any<std::string>(k_CentralMoments));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const Float32Array* omega1 = ds.getDataAs<Float32Array>(k_Omega1Path);
  const Float32Array* omega2 = ds.getDataAs<Float32Array>(k_Omega2Path);
  REQUIRE(omega1 != nullptr);
  REQUIRE(omega2 != nullptr);
  float32 value1 = (*omega1)[1];
  float32 value2 = (*omega2)[1];
  REQUIRE(std::fabs(value1 - 0.95493) < 0.00001);
  REQUIRE(std::fabs(value2 - 0.911891) < 0.00001);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: InValid Filter Execution", "[SimplnxCore][ComputeMomentInvariants2DFilter]")
{
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ComputeMomentInvariants2DFilter filter;
  DataStructure ds = CreateInvalidTestData(); // create invalid data structure input with 3D Image geometry
  Arguments args;

  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_CellData, k_FeatureIds})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData, k_RectCoords})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry, k_FeatureData})));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key, std::make_any<std::string>(k_Omega1));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key, std::make_any<std::string>(k_Omega2));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key, std::make_any<std::string>(k_CentralMoments));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeMomentInvariants2DFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeMomentInvariants2DFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeMomentInvariants2DFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeMomentInvariants2DFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeMomentInvariants2DFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeMomentInvariants2DFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeMomentInvariants2DFilter::k_FeatureRectArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<bool>(ComputeMomentInvariants2DFilter::k_NormalizeMomentInvariants_Key) == true);
      CHECK(args.value<DataPath>(ComputeMomentInvariants2DFilter::k_FeatureAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeMomentInvariants2DFilter::k_Omega1ArrayName_Key) == "TestArray");
      CHECK(args.value<std::string>(ComputeMomentInvariants2DFilter::k_Omega2ArrayName_Key) == "TestArray");
      CHECK(args.value<bool>(ComputeMomentInvariants2DFilter::k_SaveCentralMoments_Key) == true);
      CHECK(args.value<std::string>(ComputeMomentInvariants2DFilter::k_CentralMomentsArrayName_Key) == "TestArray");
    }
  }
}

TEST_CASE("SimplnxCore::ComputeMomentInvariants2DFilter: Geometry cell tuple count", "[SimplnxCore][ComputeMomentInvariants2DFilter][TupleCount]")
{
  using FilterType = ComputeMomentInvariants2DFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Grid");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({4, 3, 1});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{1, 3, 4}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  // Equal counts remain valid in another Attribute Matrix with a different tuple shape.
  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 0), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Grid"})));
  args.insertOrAssign(FilterType::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  auto* featureAMPtr = AttributeMatrix::Create(dataStructure, "Feature Data", ShapeType{2}, imageGeomPtr->getId());
  REQUIRE(featureAMPtr != nullptr);
  args.insertOrAssign(FilterType::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Feature Data"})));
  REQUIRE(DataArray<uint32>::Create(dataStructure, "Feature Rect", std::make_shared<DataStore<uint32>>(ShapeType{2}, ShapeType{6}, 0), featureAMPtr->getId()) != nullptr);
  args.insertOrAssign(FilterType::k_FeatureRectArrayPath_Key, std::make_any<DataPath>(DataPath({"Grid", "Feature Data", "Feature Rect"})));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -9511);
    REQUIRE(errorRef.message.find(inputPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", tuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Grid' has 12 cells") != std::string::npos);
  }
}

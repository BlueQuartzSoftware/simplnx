#include "SimplnxCore/Filters/ComputeFeatureNeighborsFilter.hpp"
#include "SimplnxCore/Filters/RequireMinNumNeighborsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  const bool singlePhase = GENERATE(false, true);
  DYNAMIC_SECTION("ApplyToSinglePhase=" << singlePhase)
  {
    // Class 1 oracle: feature 1 occupies the two middle cells of [2, 1, 1, 3]
    // and is rejected. Its left/right face-neighbor votes select features 2 and 3,
    // yielding FeatureIds [1, 1, 2, 2] after inactive-feature removal remaps IDs.
    DataStructure dataStructure;

    const SizeVec3 imageSize = {4, 1, 1};
    const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

    auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
    imageGeom->setDimensions(imageSize);
    auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

    auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
    auto* copiedValues = UnitTest::CreateTestDataArray<int32>(dataStructure, "CopiedValues", arraySize, {1}, cellAM->getId());
    auto* ignoredValues = UnitTest::CreateTestDataArray<int32>(dataStructure, "IgnoredValues", arraySize, {1}, cellAM->getId());
    auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
    auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, featureAM->getId());

    const std::array<int32, 4> inputFeatureIds = {2, 1, 1, 3};
    const std::array<int32, 4> inputCopiedValues = {20, 101, 102, 30};
    const std::array<int32, 4> inputIgnoredValues = {200, 101, 102, 300};
    const std::array<int32, 4> inputNumNeighbors = singlePhase ? std::array<int32, 4>{0, 0, 0, 3} : std::array<int32, 4>{0, 0, 3, 3};
    const std::array<int32, 4> inputPhases = {0, 1, 2, 1};
    for(usize i = 0; i < inputFeatureIds.size(); i++)
    {
      featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
      copiedValues->getDataStoreRef()[i] = inputCopiedValues[i];
      ignoredValues->getDataStoreRef()[i] = inputIgnoredValues[i];
      numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
      phases->getDataStoreRef()[i] = inputPhases[i];
    }

    RequireMinNumNeighborsFilter filter;
    Arguments args;
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(singlePhase));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(1));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({"ImageGeometry", "CellData", "IgnoredValues"})}));

    SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
    SIMPLNX_RESULT_REQUIRE_VALID(filter.execute(dataStructure, args).result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
    const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"}));
    const auto& outputCopiedValues = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "CopiedValues"}));
    const auto& outputIgnoredValues = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "IgnoredValues"}));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
    const auto& outputNumNeighbors = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"}));
    const auto& outputPhases = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "FeatureData", "Phases"}));
    const std::array<int32, 4> expectedFeatureIds = {1, 1, 2, 2};
    const std::array<int32, 4> expectedCopiedValues = {20, 20, 30, 30};
    const std::array<int32, 3> expectedNumNeighbors = singlePhase ? std::array<int32, 3>{0, 0, 3} : std::array<int32, 3>{0, 3, 3};
    const std::array<int32, 3> expectedPhases = {0, 2, 1};
    for(usize i = 0; i < expectedFeatureIds.size(); i++)
    {
      CHECK(outputFeatureIds[i] == expectedFeatureIds[i]);
      CHECK(outputCopiedValues[i] == expectedCopiedValues[i]);
      CHECK(outputIgnoredValues[i] == inputIgnoredValues[i]);
      // Class 4 invariants: reassignment leaves no rejected IDs and IDs remain valid.
      CHECK(outputFeatureIds[i] >= 0);
      CHECK(outputFeatureIds[i] < 3);
    }
    REQUIRE(outputNumNeighbors.getNumberOfTuples() == expectedNumNeighbors.size());
    REQUIRE(outputPhases.getNumberOfTuples() == expectedPhases.size());
    for(usize i = 0; i < expectedNumNeighbors.size(); i++)
    {
      CHECK(outputNumNeighbors[i] == expectedNumNeighbors[i]);
      CHECK(outputPhases[i] == expectedPhases[i]);
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter", "[SimplnxCore][RequireMinNumNeighborsFilter]")
{
  UnitTest::LoadPlugins();

  constexpr int32 k_MinNumNeighbors = 3;
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataAttributeMatrix = smallIn100Group.createChildPath(k_CellData);
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath cellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
  std::string numNeighborName = "NumNeighbors2";
  std::string neighborListName = "NeighborList2";
  std::string sharedSurfaceAreaListName = "SharedSurfaceAreaList2";
  std::string boundaryCellsName = "BoundaryCells_computed";
  std::string surfaceFeaturesName = "SurfaceFeatures_computed";
  DataPath numNeighborPath = cellFeatureAttributeMatrixPath.createChildPath(numNeighborName);
  DataPath neighborListPath = cellFeatureAttributeMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaListPath = cellFeatureAttributeMatrixPath.createChildPath(sharedSurfaceAreaListName);
  DataPath boundaryCellsPath = cellFeatureAttributeMatrixPath.createChildPath(boundaryCellsName);
  DataPath surfaceFeaturesPath = cellFeatureAttributeMatrixPath.createChildPath(surfaceFeaturesName);

  {
    ComputeFeatureNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_CellFeaturesPath_Key, std::make_any<DataPath>(cellFeatureAttributeMatrixPath));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreBoundary_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_BoundaryCellsName_Key, std::make_any<std::string>(boundaryCellsName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_StoreSurface_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SurfaceFeaturesName_Key, std::make_any<std::string>(surfaceFeaturesName));

    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NumNeighborsName_Key, std::make_any<std::string>(numNeighborName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_NeighborListName_Key, std::make_any<std::string>(neighborListName));
    args.insertOrAssign(ComputeFeatureNeighborsFilter::k_SharedSurfaceAreaName_Key, std::make_any<std::string>(sharedSurfaceAreaListName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  DataPath numElementsPath = cellFeatureAttributeMatrixPath.createChildPath("NumElements");
  const auto& inputNumElements = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
  const auto& computedNumNeighbors = dataStructure.getDataRefAs<Int32Array>(numNeighborPath);
  REQUIRE(inputNumElements.getNumberOfTuples() == computedNumNeighbors.getNumberOfTuples());

  // Feature 0 is retained. Other feature tuples meeting the minimum-neighbor
  // threshold are retained in their original order.
  std::vector<int32> expectedNumElements;
  expectedNumElements.reserve(inputNumElements.getNumberOfTuples());
  for(usize featureId = 0; featureId < inputNumElements.getNumberOfTuples(); featureId++)
  {
    if(featureId == 0 || computedNumNeighbors[featureId] >= k_MinNumNeighbors)
    {
      expectedNumElements.push_back(inputNumElements[featureId]);
    }
  }

  {
    RequireMinNumNeighborsFilter filter;
    Arguments args;

    args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(k_MinNumNeighbors));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(numNeighborPath));
    // args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.warnings().size() == 1);
    CHECK(preflightResult.outputActions.warnings()[0].code == -5558);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(numElementsPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == expectedNumElements.size());

    for(usize i = 0; i < expectedNumElements.size(); i++)
    {
      REQUIRE(expectedNumElements[i] == createdFeatureArray[i]);
    }
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  {
    // Write out the DataStructure for later viewing/debugging
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileIO::WriteFile(fmt::format("{}/minimum_neighbors_test.dream3d", unit_test::k_BinaryTestOutputDir));
    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // With "Apply to Single Phase Only" enabled, preflight validates that the feature-level
  // NumNeighbors and FeaturePhases arrays share the same tuple count. Build them with
  // deliberately different tuple counts (in separate AttributeMatrices) to drive the
  // validateNumberOfTuples() guard that emits error -252.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({4, 1, 1});

  // Cell Data: FeatureIds is dereferenced in preflight (must exist) but is not part of the
  // feature-level tuple-count check.
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {4}, {1}, cellAM->getId());

  // Feature Data: NumNeighbors has 5 tuples.
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {5}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {5}, {1}, featureAM->getId());

  // FeaturePhases lives in a separate AttributeMatrix with a different tuple count (4 != 5),
  // causing the cross-array tuple-count check to fail.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, mismatchAM->getId());

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -252);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)", "[SimplnxCore][RequireMinNumNeighborsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {1, 1, 4}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", {1, 1, 4}, {1}, cellAM->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -55571);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {1, 4, 2, 3};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 3, 3};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -55567);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {3}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {3}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {-1, 1, 1, 2};
  const std::array<int32, 3> inputNumNeighbors = {0, 3, 3};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    if(i < inputNumNeighbors.size())
    {
      numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
    }
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  const auto& outputFeatureIds = dataStructure.getDataRefAs<Int32Array>(DataPath({"ImageGeometry", "CellData", "FeatureIds"}));
  const std::array<int32, 4> expectedFeatureIds = {1, 1, 1, 2};
  for(usize i = 0; i < expectedFeatureIds.size(); i++)
  {
    CHECK(outputFeatureIds[i] == expectedFeatureIds[i]);
  }
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {2, 1, 1, 3};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 3, 3};
  const std::array<int32, 4> inputPhases = {0, 1, 2, 1};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
    phases->getDataStoreRef()[i] = inputPhases[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(2));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(true));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(5));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "Phases"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -5555);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType arraySize(std::reverse_iterator(imageSize.end()), std::reverse_iterator(imageSize.begin()));

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", arraySize, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {4}, imageGeom->getId());

  auto* featureIds = UnitTest::CreateTestDataArray<int32>(dataStructure, "FeatureIds", arraySize, {1}, cellAM->getId());
  auto* numNeighbors = UnitTest::CreateTestDataArray<int32>(dataStructure, "NumNeighbors", {4}, {1}, featureAM->getId());
  const std::array<int32, 4> inputFeatureIds = {1, 2, 3, 1};
  const std::array<int32, 4> inputNumNeighbors = {0, 0, 0, 0};
  for(usize i = 0; i < inputFeatureIds.size(); i++)
  {
    featureIds->getDataStoreRef()[i] = inputFeatureIds[i];
    numNeighbors->getDataStoreRef()[i] = inputNumNeighbors[i];
  }

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(1));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  SIMPLNX_RESULT_REQUIRE_VALID(filter.preflight(dataStructure, args).outputActions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -55569);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - no coarsening progress (-55572)", "[SimplnxCore][RequireMinNumNeighborsFilter][execute]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;

  const SizeVec3 imageSize = {4, 1, 1};
  const ShapeType cellShape = {1, 1, 4};

  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageGeometry");
  imageGeom->setDimensions(imageSize);
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* featureAM = AttributeMatrix::Create(dataStructure, "FeatureData", {2}, imageGeom->getId());

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsStore, cellAM->getId());

  auto numNeighborsStore = DataStoreUtilities::CreateDataStore<int32>({2}, {1}, IDataAction::Mode::Execute);
  auto* numNeighbors = DataArray<int32>::Create(dataStructure, "NumNeighbors", numNeighborsStore, featureAM->getId());

  featureIds->fill(-1);
  numNeighbors->fill(0);
  numNeighbors->getDataStoreRef()[1] = 5;

  RequireMinNumNeighborsFilter filter;
  Arguments args;
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key, std::make_any<uint64>(3));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key, std::make_any<bool>(false));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_PhaseNumber_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "CellData", "FeatureIds"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key, std::make_any<DataPath>(DataPath({"ImageGeometry", "FeatureData", "NumNeighbors"})));
  args.insertOrAssign(RequireMinNumNeighborsFilter::k_IgnoredVoxelArrays_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  CHECK(executeResult.result.errors()[0].code == -55572);

  for(usize i = 0; i < featureIds->getNumberOfTuples(); i++)
  {
    CHECK((*featureIds)[i] == -1);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::RequireMinNumNeighborsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][RequireMinNumNeighborsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "RequireMinNumNeighborsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "RequireMinNumNeighborsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<RequireMinNumNeighborsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<uint64>(RequireMinNumNeighborsFilter::k_MinNumNeighbors_Key) == 5);
      CHECK(args.value<bool>(RequireMinNumNeighborsFilter::k_ApplyToSinglePhase_Key) == true);
      CHECK(args.value<uint64>(RequireMinNumNeighborsFilter::k_PhaseNumber_Key) == 5);
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_FeatureIdsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_FeaturePhasesPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(RequireMinNumNeighborsFilter::k_NumNeighborsPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

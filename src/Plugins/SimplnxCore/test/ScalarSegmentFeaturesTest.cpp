#include "SimplnxCore/Filters/ScalarSegmentFeaturesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

namespace
{
const std::string k_SharedEdgesInputArrayName = "Shared Edges";
const std::string k_SharedPointsInputArrayName = "Shared Points";
const std::string k_NothingSharedInputArrayName = "Nothing Shared";
const std::string k_CombinationInputArrayName = "Combination";
const std::string k_ExemplarySharedEdgesFaceOnlyFeatureIdsName = "Exemplary Shared Edges FeatureIds - Face Only";
const std::string k_ExemplarySharedEdgesAllConnectedFeatureIdsName = "Exemplary Shared Edges FeatureIds - All Connected";
const std::string k_ExemplarySharedPointsFaceOnlyFeatureIdsName = "Exemplary Shared Points FeatureIds - Face Only";
const std::string k_ExemplarySharedPointsAllConnectedFeatureIdsName = "Exemplary Shared Points FeatureIds - All Connected";
const std::string k_ExemplaryNothingSharedFaceOnlyFeatureIdsName = "Exemplary Nothing Shared FeatureIds - Face Only";
const std::string k_ExemplaryNothingSharedAllConnectedFeatureIdsName = "Exemplary Nothing Shared FeatureIds - All Connected";
const std::string k_ExemplaryCombinationFaceOnlyFeatureIdsName = "Exemplary Combination FeatureIds - Face Only";
const std::string k_ExemplaryCombinationAllConnectedFeatureIdsName = "Exemplary Combination FeatureIds - All Connected";
} // namespace

TEST_CASE("SimplnxCore::ScalarSegmentFeatures", "[SimplnxCore][ScalarSegmentFeatures]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_test_data_1_v2.tar.gz", "6_5_test_data_1_v2");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1_v2/6_5_test_data_1_v2.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    Arguments args;
    ScalarSegmentFeaturesFilter filter;

    DataPath smallIn100Group({k_DataContainer});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_CellData);
    DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(k_FeatureIds);
    std::string outputFeatureIdsName = "Output_Feature_Ids";
    std::string computedCellDataName = "Computed_CellData";
    DataPath outputFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
    DataPath featureDataGroupPath = smallIn100Group.createChildPath(computedCellDataName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_DataContainer});
    int scalarTolerance = 0;

    // Create default Parameters for the filter.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    // Turn off the use of a Mask Array
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    // Set the input array and the tolerance
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    // Set the paths to the created arrays
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>(outputFeatureIdsName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>(computedCellDataName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    // Are we going to randomize the featureIds when completed.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(activeArrayDataPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 847);
  }

  {
    // Write out the DataStructure for later viewing/debugging
    std::string filePath = fmt::format("{}/ScalarSegmentFeatures.dream3d", unit_test::k_BinaryTestOutputDir);
    // std::cout << "Writing file to: " << filePath << std::endl;
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);

    auto resultH5 = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Neighbor Scheme", "[Reconstruction][ScalarSegmentFeatures]")
{
  /**
   * We are going to use Catch2's GENERATE macro to create variations of parameter values.
   * EVERYTHING after the GENERATE macro will be run for each of the generated sets of values
   */
  auto [sectionName, inputDataArrayName, exemplaryFeatureIdsArrayName, neighborSchemeIndex] =
      GENERATE(std::make_tuple("Shared Edges - Face Only", k_SharedEdgesInputArrayName, k_ExemplarySharedEdgesFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Shared Edges - All Connected", k_SharedEdgesInputArrayName, k_ExemplarySharedEdgesAllConnectedFeatureIdsName, 1),
               std::make_tuple("Shared Points - Face Only", k_SharedPointsInputArrayName, k_ExemplarySharedPointsFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Shared Points - All Connected", k_SharedPointsInputArrayName, k_ExemplarySharedPointsAllConnectedFeatureIdsName, 1),
               std::make_tuple("Nothing Shared - Face Only", k_NothingSharedInputArrayName, k_ExemplaryNothingSharedFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Nothing Shared - All Connected", k_NothingSharedInputArrayName, k_ExemplaryNothingSharedAllConnectedFeatureIdsName, 1),
               std::make_tuple("Combination - Face Only", k_CombinationInputArrayName, k_ExemplaryCombinationFaceOnlyFeatureIdsName, 0),
               std::make_tuple("Combination - All Connected", k_CombinationInputArrayName, k_ExemplaryCombinationAllConnectedFeatureIdsName, 1));

  /**
   * @note EVERYTHING from here to the end of the test will be run for **each** tuple set above
   */
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_neighbor_scheme_test.tar.gz", "segment_features_neighbor_scheme_test");
  auto baseDataFilePath = fs::path(fmt::format("{}/segment_features_neighbor_scheme_test/segment_features_neighbor_scheme_test.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  {
    Arguments args;
    ScalarSegmentFeaturesFilter filter;

    DataPath smallIn100Group({k_SmallIn100ImageGeom});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_Cell_Data);
    std::string outputFeatureIdsName = "Output_Feature_Ids";
    std::string computedCellDataName = "Computed_CellData";
    DataPath outputFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
    DataPath featureDataGroupPath = smallIn100Group.createChildPath(computedCellDataName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_SmallIn100ImageGeom});
    int scalarTolerance = 0;

    // Create default Parameters for the filter.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    // Turn off the use of a Mask Array
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    // Set the tolerance
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    // Set the paths to the created arrays
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>(outputFeatureIdsName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>(computedCellDataName));
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    // Are we going to randomize the featureIds when completed.
    args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));

    SECTION(sectionName)
    {
      DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(inputDataArrayName);
      DataPath exemplaryFeatureIdsArrayPath = ebsdScanDataPath.createChildPath(exemplaryFeatureIdsArrayName);
      DataPath computedFeatureIdsPath = ebsdScanDataPath.createChildPath(outputFeatureIdsName);
      args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
      args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(neighborSchemeIndex));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      UnitTest::CompareArrays<int32>(dataStructure, exemplaryFeatureIdsArrayPath, computedFeatureIdsPath);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Masked Voxel 0 Seed Validation", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Regression pin for the shared SegmentFeatures driver: the first seed must be validated (and
  // stamped) by getSeed() exactly like every later seed. 5x1x1 with voxel 0 masked out; scalar
  // values [9, 5, 5, 9, 7] at tolerance 1 give F1 = {1, 2} and F2 = {4}; masked cells keep
  // FeatureId 0. A driver that bursts from the raw index 0 produces a phantom empty feature 1
  // and shifted ids [0, 2, 2, 0, 3].
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* scalarArrayPtr = CreateTestDataArray<int32>(dataStructure, "ScalarValues", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* maskArrayPtr = CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 5}, {1}, cellAM->getId());

  const std::vector<int32> scalarValues = {9, 5, 5, 9, 7};
  const std::vector<bool> maskValues = {false, true, true, false, true};
  for(usize cellIdx = 0; cellIdx < scalarValues.size(); cellIdx++)
  {
    (*scalarArrayPtr)[cellIdx] = scalarValues[cellIdx];
    (*maskArrayPtr)[cellIdx] = maskValues[cellIdx];
  }

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int32>(1));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "ScalarValues"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})));
  const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})).getDataStoreRef();
  const std::vector<int32> expectedFeatureIds = {0, 1, 1, 0, 2};
  for(usize cellIdx = 0; cellIdx < expectedFeatureIds.size(); cellIdx++)
  {
    INFO(fmt::format("cell index {}", cellIdx));
    REQUIRE(featureIdsRef[cellIdx] == expectedFeatureIds[cellIdx]);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(DataPath({"Geometry", "CellFeatureData", "Active"})));
  REQUIRE(dataStructure.getDataRefAs<UInt8Array>(DataPath({"Geometry", "CellFeatureData", "Active"})).getNumberOfTuples() == 3);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ScalarSegmentFeatures: Execute Error - All Cells Masked (-87000)", "[SimplnxCore][ScalarSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Regression pin for the shared SegmentFeatures driver: with every cell masked out no seed
  // exists, so the filter must fail with -87000. The pre-fix driver burst from the raw index 0
  // and "succeeded" with one phantom, zero-cell feature.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({3, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 3}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* scalarArrayPtr = CreateTestDataArray<int32>(dataStructure, "ScalarValues", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* maskArrayPtr = CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  for(usize cellIdx = 0; cellIdx < 3; cellIdx++)
  {
    (*scalarArrayPtr)[cellIdx] = 5;
    (*maskArrayPtr)[cellIdx] = false;
  }

  ScalarSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ScalarToleranceKey, std::make_any<int32>(1));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_InputArrayPathKey, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "ScalarValues"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_FeatureIdsName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_CellFeatureName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));
  args.insertOrAssign(ScalarSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}

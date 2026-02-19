#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace ebsd_segment_features_constants
{
inline constexpr StringLiteral k_InputGeometryName = "DataContainer";
inline const DataPath k_InputGeometryPath({k_InputGeometryName});
inline constexpr StringLiteral k_CellDataName = "CellData";
inline constexpr StringLiteral k_EnsembleName = "CellEnsembleData";
inline const DataPath k_QuatsArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Quats");
inline const DataPath k_PhasesArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Phases");
inline const DataPath k_MaskArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Mask (Y Pos)");

inline const DataPath k_CrystalStructuresArrayPath = k_InputGeometryPath.createChildPath(k_EnsembleName).createChildPath("CrystalStructures");

inline const DataPath k_ActivesArrayPath = k_InputGeometryPath.createChildPath(k_Grain_Data).createChildPath(k_ActiveName);

inline const DataPath k_FeatureIdsArrayPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath(k_FeatureIds);

inline const DataPath k_FeatureIdsFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Face");
inline const DataPath k_FeatureIdsAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_All");
inline const DataPath k_FeatureIdsMaskFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Mask_Face");
inline const DataPath k_FeatureIdsMaskAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("Ebsd_FeatureIds_Mask_All");
} // namespace ebsd_segment_features_constants

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:Face", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 83);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:All", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 77);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:MaskFace", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 36);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsMaskFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures:MaskAll", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(ebsd_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(ebsd_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 32);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(ebsd_segment_features_constants::k_FeatureIdsMaskAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

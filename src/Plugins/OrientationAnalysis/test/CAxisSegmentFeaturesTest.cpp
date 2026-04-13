#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
namespace caxis_segment_features_constants
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

inline const DataPath k_FeatureIdsFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Face");
inline const DataPath k_FeatureIdsAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_All");
inline const DataPath k_FeatureIdsMaskFacePath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Mask_Face");
inline const DataPath k_FeatureIdsMaskAllPath = k_InputGeometryPath.createChildPath(k_CellDataName).createChildPath("CAxis_FeatureIds_Mask_All");
} // namespace caxis_segment_features_constants

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:Face", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 57);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:All", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 37);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:MaskFace", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 31);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsMaskFacePath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeatures:MaskAll", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "segment_features_test_data.tar.gz", "segment_features_test_data");
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/segment_features_test_data/segment_features_test_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_MaskArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_InputGeometryPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(caxis_segment_features_constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    UInt8Array& actives = dataStructure.getDataRefAs<UInt8Array>(caxis_segment_features_constants::k_ActivesArrayPath);
    size_t numFeatures = actives.getNumberOfTuples();
    REQUIRE(numFeatures == 25);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = dataStructure.getDataRefAs<Int32Array>(caxis_segment_features_constants::k_FeatureIdsMaskAllPath);

    UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][CAxisSegmentFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CAxisSegmentFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CAxisSegmentFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CAxisSegmentFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<float32>(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<bool>(CAxisSegmentFeaturesFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key) == true);
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key) == "TestName");
    }
  }
}

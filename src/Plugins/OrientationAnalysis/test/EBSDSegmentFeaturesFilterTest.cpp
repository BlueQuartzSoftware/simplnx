#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <cmath>
#include <filesystem>
#include <fstream>

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

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][EBSDSegmentFeaturesFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "EBSDSegmentFeaturesFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "EBSDSegmentFeaturesFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<EBSDSegmentFeaturesFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key) == true);
      }
      CHECK(args.value<float32>(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<bool>(EBSDSegmentFeaturesFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key) == "TestName");
    }
  }
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Masked Voxel 0 Seed Validation", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Regression pin for the shared SegmentFeatures driver: the first seed must be validated (and
  // stamped) by getSeed() exactly like every later seed. 5x1x1 with voxel 0 masked out; per-cell
  // orientations are pure rotations about x by Phi = [0, 20, 22, 0, 90] degrees, so pairwise
  // misorientations equal |dPhi|. At tolerance 10 the expected features are F1 = {1, 2} (2 deg)
  // and F2 = {4}; masked cells keep FeatureId 0. A driver that bursts from the raw index 0
  // produces a phantom empty feature 1 and shifted ids [0, 2, 2, 0, 3].
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
  imageGeom->setDimensions({5, 1, 1});
  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 5}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 5}, {4}, cellAM->getId());
  auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 5}, {1}, cellAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
  auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

  const std::vector<float32> phiDegrees = {0.0f, 20.0f, 22.0f, 0.0f, 90.0f};
  const std::vector<bool> maskValues = {false, true, true, false, true};
  for(usize cellIdx = 0; cellIdx < phiDegrees.size(); cellIdx++)
  {
    const float32 halfAngleRad = (phiDegrees[cellIdx] * 0.5f) * Constants::k_PiOver180F;
    (*quatsArrayPtr)[cellIdx * 4 + 0] = std::sin(halfAngleRad);
    (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 3] = std::cos(halfAngleRad);
    (*phasesArrayPtr)[cellIdx] = 1;
    (*maskArrayPtr)[cellIdx] = maskValues[cellIdx];
  }
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

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

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Periodic Boundary Wrap", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  UnitTest::LoadPlugins();

  // Regression pin for the IsPeriodic parameter (previously a silent no-op in the shared
  // SegmentFeatures driver). 4x1x1 line of pure rotations about x by Phi = [0, 30, 30, 2]
  // degrees (Hexagonal_High): pairwise misorientations equal |dPhi|, so at tolerance 10 the
  // non-periodic partition is {0} {1,2} {3}; periodic, the x boundary wraps and the end cells
  // (misorientation 2 degrees) join: {0,3} {1,2}.
  auto runFilter = [](bool isPeriodic) -> std::vector<int32> {
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, "Geometry");
    imageGeom->setDimensions({4, 1, 1});
    auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", ShapeType{1, 1, 4}, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 4}, {4}, cellAM->getId());
    auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 4}, {1}, cellAM->getId());
    auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
    auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

    const std::vector<float32> phiDegrees = {0.0f, 30.0f, 30.0f, 2.0f};
    for(usize cellIdx = 0; cellIdx < phiDegrees.size(); cellIdx++)
    {
      const float32 halfAngleRad = (phiDegrees[cellIdx] * 0.5f) * Constants::k_PiOver180F;
      (*quatsArrayPtr)[cellIdx * 4 + 0] = std::sin(halfAngleRad);
      (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
      (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
      (*quatsArrayPtr)[cellIdx * 4 + 3] = std::cos(halfAngleRad);
      (*phasesArrayPtr)[cellIdx] = 1;
    }
    (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
    (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

    EBSDSegmentFeaturesFilter filter;
    Arguments args;
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})));
    const auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({"Geometry", "CellData", "FeatureIds"})).getDataStoreRef();
    std::vector<int32> featureIds(featureIdsRef.getNumberOfTuples());
    for(usize cellIdx = 0; cellIdx < featureIds.size(); cellIdx++)
    {
      featureIds[cellIdx] = featureIdsRef[cellIdx];
    }
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
    return featureIds;
  };

  REQUIRE(runFilter(false) == std::vector<int32>{1, 2, 2, 3});
  REQUIRE(runFilter(true) == std::vector<int32>{1, 2, 2, 1});
}

TEST_CASE("OrientationAnalysis::EBSDSegmentFeaturesFilter: Execute Error - All Cells Masked (-87000)", "[OrientationAnalysis][EBSDSegmentFeatures]")
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
  auto* quatsArrayPtr = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", ShapeType{1, 1, 3}, {4}, cellAM->getId());
  auto* phasesArrayPtr = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", ShapeType{1, 1, 3}, {1}, cellAM->getId());
  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", ShapeType{2}, imageGeom->getId());
  auto* crystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

  for(usize cellIdx = 0; cellIdx < 3; cellIdx++)
  {
    (*quatsArrayPtr)[cellIdx * 4 + 0] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 1] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 2] = 0.0f;
    (*quatsArrayPtr)[cellIdx * 4 + 3] = 1.0f;
    (*phasesArrayPtr)[cellIdx] = 1;
    (*maskArrayPtr)[cellIdx] = false;
  }
  (*crystalStructuresArrayPtr)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructuresArrayPtr)[1] = ebsdlib::CrystalStructure::Hexagonal_High;

  EBSDSegmentFeaturesFilter filter;
  Arguments args;
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(10.0F));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_NeighborScheme_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Mask"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Geometry"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Quats"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellData", "Phases"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Geometry", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>("FeatureIds"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>("CellFeatureData"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>("Active"));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(EBSDSegmentFeaturesFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -87000);
}

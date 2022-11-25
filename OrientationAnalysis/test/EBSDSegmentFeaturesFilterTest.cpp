#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include "OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex_plugins/EbsdLibConstants.hpp"
#include "complex_plugins/Utilities/SmallIN100Utilties.hpp"
#include "complex_plugins/Utilities/TestUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::UnitTest;

TEST_CASE("OrientationAnalysis::EBSDSegmentFeatures: Instantiation and Parameter Check", "[OrientationAnalysis][EBSDSegmentFeatures]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/6_6_ebsd_segment_features.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/Small_IN100.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteAlignSectionsMisorientation(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir)));

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Align Sections Feature Centroid Filter
  SmallIn100::ExecuteAlignSectionsFeatureCentroid(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir)));

  // Bad Data Neighbor Orientation Check Filter
  SmallIn100::ExecuteBadDataNeighborOrientationCheck(dataStructure, *filterList);

  // Neighbor Orientation Correlation Filter
  SmallIn100::ExecuteNeighborOrientationCorrelation(dataStructure, *filterList);

  // EBSD Segment Features/Semgent Features (Misorientation) Filter
  {
    EBSDSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_UseGoodVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_GridGeomPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_GoodVoxelsPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_GrainData));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_Active));
    args.insertOrAssign(EBSDSegmentFeaturesFilter::k_RandomizeFeatures_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedDataArray = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsArrayPath);
    const auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<Int32Array>(DataPath({"Small IN100", k_SmallIN100ScanData, k_FeatureIds}));

    CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
  }
}

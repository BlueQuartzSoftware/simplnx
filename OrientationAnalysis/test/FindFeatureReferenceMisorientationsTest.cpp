#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindFeatureReferenceMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex_plugins/Utilities/TestUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::UnitTest;

namespace
{
const std::string k_FeatureReferenceMisorientationsArrayName("FeatureReferenceMisorientations");
const std::string k_GBEuclideanDistancesArrayPath("FeatureAvgMisorientations");
} // namespace

TEST_CASE("OrientationAnalysis::FindFeatureReferenceMisorientationsFilter", "[OrientationAnalysis][FindFeatureReferenceMisorientationsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_stats_test.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);

  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath featurePhasesDataPath = cellFeatureDataPath.createChildPath(k_Phases);

  // Instantiate the filter and an Arguments Object
  {
    FindFeatureReferenceMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));

    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsDataPath));
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_FeatureReferenceMisorientationsArrayName_Key, std::make_any<DataPath>({k_FeatureReferenceMisorientationsArrayName}));
    args.insertOrAssign(FindFeatureReferenceMisorientationsFilter::k_FeatureAvgMisorientationsArrayName_Key, std::make_any<DataPath>({k_GBEuclideanDistancesArrayPath}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_FeatureReferenceMisorientationsArrayName});
    const DataPath k_ExemplarArrayPath({k_FeatureReferenceMisorientationsArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Compare the Output Feature Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellFeatureData, k_GBEuclideanDistancesArrayPath});
    const DataPath k_ExemplarArrayPath({k_GBEuclideanDistancesArrayPath});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_reference_misorientations.dream3d", unit_test::k_BinaryTestOutputDir)));
}

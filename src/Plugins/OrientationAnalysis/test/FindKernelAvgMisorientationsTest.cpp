#include "OrientationAnalysis/Filters/FindKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_KernelAverageMisorientationsArrayName_Exemplar("KernelAverageMisorientations");
const std::string k_KernelAverageMisorientationsArrayName("CalculatedKernelAverageMisorientations");
} // namespace

TEST_CASE("OrientationAnalysis::FindKernelAvgMisorientationsFilter", "[OrientationAnalysis][FindKernelAvgMisorientationsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_stats_test.tar.gz", "6_6_stats_test.dream3d");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindKernelAvgMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    // Parameters
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_KernelSize_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{1, 1, 1}));
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(smallIn100Group));
    // Cell Arrays
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    // Ensemble Arrays
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    // Output Array
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_KernelAverageMisorientationsArrayName_Key, std::make_any<std::string>(k_KernelAverageMisorientationsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName_Exemplar});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_kernel_average_misorientations.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

#include "OrientationAnalysis/Filters/FindKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const std::string k_KernelAverageMisorientationsArrayName_Exemplar("KernelAverageMisorientations");
const std::string k_KernelAverageMisorientationsArrayName("CalculatedKernelAverageMisorientations");
} // namespace

TEST_CASE("OrientationAnalysis::FindKernelAvgMisorientationsFilter", "[OrientationAnalysis][FindKernelAvgMisorientationsFilter]")
{
  const std::string kDataInputArchive = "6_6_stats_test.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_stats_test.dream3d";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_stats_test.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindKernelAvgMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    // Parameters
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_KernelSize_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{1, 1, 1}));
    args.insertOrAssign(FindKernelAvgMisorientationsFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(smallIn100Group));
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
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellData, k_KernelAverageMisorientationsArrayName_Exemplar});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_kernel_average_misorientations.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

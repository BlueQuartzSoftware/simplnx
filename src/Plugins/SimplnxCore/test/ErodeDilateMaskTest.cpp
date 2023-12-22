#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_EbsdScanDataName("EBSD Scan Data");

const DataPath k_InputData({"Input Data"});
const DataPath k_EbsdScanDataDataPath = k_InputData.createChildPath(k_EbsdScanDataName);
const DataPath k_MaskArrayDataPath = k_EbsdScanDataDataPath.createChildPath("Mask");

} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Dilate)", "[SimplnxCore][ErodeDilateMaskFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  const std::string k_ExemplarDataContainerName("Exemplar Mask Dilate");
  const DataPath k_DilateCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, "EBSD Scan Data"});

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_mask.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    const ErodeDilateMaskFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Dilate));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayDataPath));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_InputData));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataDataPath, k_ExemplarDataContainerName);
}

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter(Erode)", "[SimplnxCore][ErodeDilateMaskFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_erode_dilate_test.tar.gz", "6_6_erode_dilate_test");

  const std::string k_ExemplarDataContainerName("Exemplar Mask Erode");
  const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarDataContainerName, "EBSD Scan Data"});

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_mask.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    const ErodeDilateMaskFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateMaskFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
    args.insertOrAssign(ErodeDilateMaskFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateMaskFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateMaskFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayDataPath));
    args.insertOrAssign(ErodeDilateMaskFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_InputData));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_EbsdScanDataDataPath, k_ExemplarDataContainerName);
}

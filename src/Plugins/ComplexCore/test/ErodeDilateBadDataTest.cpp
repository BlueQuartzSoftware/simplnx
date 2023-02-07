#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ErodeDilateBadDataFilter.hpp"

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Erode = 0ULL;
constexpr ChoicesParameter::ValueType k_Dilate = 1ULL;

const DataPath k_FeatureIdsDataPath = DataPath({"Input Data", "EBSD Scan Data", "FeatureIds"});
const DataPath k_SelectedGeometry = DataPath({"Input Data"});
const std::string k_ExemplarCoordinationNumberDataPath("Exemplar Bad Data Erode");
const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarCoordinationNumberDataPath, "EBSD Scan Data"});

const std::string k_ExemplarDilateDataContainer("Exemplar Bad Data Dilate");
const DataPath k_DilateCellAttributeMatrixDataPath = DataPath({k_ExemplarDilateDataContainer, "EBSD Scan Data"});

} // namespace

TEST_CASE("ComplexCore::ErodeDilateBadDataFilter(Erode)", "[ComplexCore][ErodeDilateBadDataFilter]")
{

  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_bad_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    const ErodeDilateBadDataFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Erode));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_ErodeCellAttributeMatrixDataPath, k_ExemplarCoordinationNumberDataPath);
}

TEST_CASE("ComplexCore::ErodeDilateBadDataFilter(Dilate)", "[ComplexCore][ErodeDilateBadDataFilter]")
{

  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_bad_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    const ErodeDilateBadDataFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(k_Dilate));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(2));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(true));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_DilateCellAttributeMatrixDataPath, k_ExemplarCoordinationNumberDataPath);
}

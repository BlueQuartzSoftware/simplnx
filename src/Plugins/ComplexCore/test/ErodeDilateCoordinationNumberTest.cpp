#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ErodeDilateCoordinationNumberFilter.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const DataPath k_FeatureIdsDataPath = DataPath({"Input Data", "EBSD Scan Data", "FeatureIds"});

const DataPath k_SelectedGeometry = DataPath({"Input Data"});
const std::string k_ExemplarErodeDataPath("Exemplar Coordination Number");
const DataPath k_ErodeCellAttributeMatrixDataPath = DataPath({k_ExemplarErodeDataPath, "EBSD Scan Data"});
} // namespace

TEST_CASE("ComplexCore::ErodeDilateCoordinationNumberFilter", "[ComplexCore][ErodeDilateCoordinationNumberFilter]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_erode_dilate_test/6_6_erode_dilate_coordination_number.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ErodeDilateCoordinationNumberFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CoordinationNumber_Key, std::make_any<int32>(6));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_Loop_Key, std::make_any<bool>(false));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsDataPath));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ErodeDilateCoordinationNumberFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  UnitTest::CompareExemplarToGeneratedData(dataStructure, dataStructure, k_ErodeCellAttributeMatrixDataPath, k_ExemplarErodeDataPath);
}

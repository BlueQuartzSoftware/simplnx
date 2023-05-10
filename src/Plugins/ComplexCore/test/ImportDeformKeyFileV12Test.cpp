#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ImportDeformKeyFileV12Filter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const DataPath k_QuadGeomPath = DataPath({k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(k_CellData);
const DataPath k_VertexPath = k_QuadGeomPath.createChildPath(k_VertexData);
} // namespace

TEST_CASE("ComplexCore::ImportDeformKeyFileV12", "[Core][ImportDeformKeyFileV12]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  // auto exemplarFilePath = fs::path(fmt::format("{}/6_6_fill_bad_data/6_6_exemplar.dream3d", unit_test::k_TestFilesDir));
  auto exemplarFilePath = fs::path("");
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  // auto baseDataFilePath = fs::path(fmt::format("{}/6_6_fill_bad_data/6_6_input.dream3d", unit_test::k_TestFilesDir));
  auto baseDataFilePath = fs::path("");
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ImportDeformKeyFileV12Filter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ImportDeformKeyFileV12Filter::k_InputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("")));

    args.insertOrAssign(ImportDeformKeyFileV12Filter::k_QuadGeomPath_Key, std::make_any<DataPath>(k_QuadGeomPath));
    args.insertOrAssign(ImportDeformKeyFileV12Filter::k_VertexAMName_Key, std::make_any<std::string>(k_VertexPath.getTargetName()));
    args.insertOrAssign(ImportDeformKeyFileV12Filter::k_CellAMName_Key, std::make_any<std::string>(k_CellPath.getTargetName()));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_VertexPath, k_QuadGeomPath.getTargetName());
  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_CellPath, k_QuadGeomPath.getTargetName());

// Write the DataStructure out to the file system
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_fill_bad_data.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
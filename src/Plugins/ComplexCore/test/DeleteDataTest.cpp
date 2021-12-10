#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/Filters/DeleteData.hpp"

using namespace complex;
using namespace complex::Constants;

namespace fs = std::filesystem;

namespace CreateImageGeometryUnitTest
{

const fs::path k_DataDir = "test/data";
const fs::path k_TestFile = "CreateImageGeometry_Test.dream3d";

} // namespace CreateImageGeometryUnitTest

TEST_CASE("ComplexCore::DeleteDataArray", "[ComplexCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataGraph = complex::UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne, k_Int8DataSet});
  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath));

  DeleteData filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataGraph.getData(selectedDataGroupPath);
  REQUIRE(removedDataArray == nullptr);
}

TEST_CASE("ComplexCore::Delete Data Group", "[ComplexCore][DeleteData]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {0.0F, 22.0F, 77.0F};

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataGraph = complex::UnitTest::CreateAllPrimitiveTypes(imageDims);

  DataPath selectedDataGroupPath({k_LevelZero, k_LevelOne});
  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(DeleteData::k_DataPath_Key, std::make_any<DataPath>(selectedDataGroupPath));

  DeleteData filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  DataObject* removedDataArray = dataGraph.getData(selectedDataGroupPath);
  REQUIRE(removedDataArray != nullptr);
}

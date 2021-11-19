/**
 * This file is auto generated from the original Core/CreateImageGeometry
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[CreateImageGeometry][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

#include "UnitTestCommon.hpp"

//#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateImageGeometry.hpp"

using namespace complex;
using namespace ComplexUnitTest;
namespace fs = std::filesystem;

namespace CreateImageGeometryUnitTest
{

const fs::path k_DataDir = "test/data";
const fs::path k_TestFile = "CreateImageGeometry_Test.dream3d";

} // namespace CreateImageGeometryUnitTest

TEST_CASE("ComplexCore::CreateImageGeometry: Instantiation and Parameter Check", "[ComplexCore]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  std::vector<usize> imageDims = {40, 60, 80};
  std::vector<float32> imageSpacing = {0.10F, 2.0F, 33.0F};
  std::vector<float32> imageOrigin = {
      0.0F,
      22.0F,
      77.0F,
  };

  VectorUInt64Parameter::ValueType inputDims = {imageDims[0], imageDims[1], imageDims[2]};

  DataStructure dataGraph = ComplexUnitTest::CreateAllPrimitiveTypes(imageDims);
  //  DataGroup* levelZeroGroup = DataGroup::Create(dataGraph, ComplexUnitTest::Constants::k_LevelZero);
  //  DataGroup* levelOneGroup = DataGroup::Create(dataGraph, ComplexUnitTest::Constants::k_LevelOne, levelZeroGroup->getId());

  DataPath selectedDataGroupPath({ComplexUnitTest::Constants::k_LevelZero, ComplexUnitTest::Constants::k_LevelOne});
  Arguments args;
  // Create default Parameters for the filter.
  args.insertOrAssign(CreateImageGeometry::k_SelectedDataGroup_Key, std::make_any<DataPath>(selectedDataGroupPath));
  args.insertOrAssign(CreateImageGeometry::k_GeometryName_Key, std::make_any<std::string>("Image Geometry"));
  args.insertOrAssign(CreateImageGeometry::k_Dimensions_Key, std::make_any<std::vector<uint64_t>>(inputDims));
  args.insertOrAssign(CreateImageGeometry::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageOrigin));
  args.insertOrAssign(CreateImageGeometry::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(imageSpacing));

  CreateImageGeometry filter;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());

  // Execute the filter and check the result
  executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid() == false);

#if 0
  fs::path filePath = COMPLEX_BUILD_DIR / CreateImageGeometryUnitTest::k_DataDir / CreateImageGeometryUnitTest::k_TestFile;

  H5::FileWriter::ResultType result = H5::FileWriter::CreateFile(filePath);
  REQUIRE(result.valid());

  H5::FileWriter& fileWriter = *(result.value());
  REQUIRE(fileWriter.isValid());

  herr_t err;
  err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
#endif
}

// TEST_CASE("Core::CreateImageGeometry: Valid filter execution")
//{
//
//}

// TEST_CASE("Core::CreateImageGeometry: InValid filter execution")
//{
//
//}

#include <catch2/catch.hpp>

#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/VtkRectilinearGridWriterFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("ComplexCore::VtkRectilinearGridWriterFilter: Valid Filter Execution", "[ComplexCore][VtkRectilinearGridWriterFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  VtkRectilinearGridWriterFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(VtkRectilinearGridWriterFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/File/To/Write.data")));
  args.insertOrAssign(VtkRectilinearGridWriterFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(VtkRectilinearGridWriterFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(VtkRectilinearGridWriterFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::VtkRectilinearGridWriterFilter: InValid Filter Execution")
//{
//
// }

#include <catch2/catch.hpp>

#include "ComplexCore/Filters/WriteStlFileFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

TEST_CASE("ComplexCore::WriteStlFileFilter: Valid Filter Execution","[ComplexCore][WriteStlFileFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupByFeature, std::make_any<bool>(false));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(unit_test::k_BinaryTestOutputDir)));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Triangle"));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteStlFileFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteStlFileFilter::k_FaceNormalsPath_Key, std::make_any<DataPath>(DataPath{}));


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

TEST_CASE("ComplexCore::WriteStlFileFilter: InValid Filter Execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupByFeature, std::make_any<bool>(false));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(unit_test::k_BinaryTestOutputDir)));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Triangle"));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteStlFileFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WriteStlFileFilter::k_FaceNormalsPath_Key, std::make_any<DataPath>(DataPath{}));


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(!preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(!executeResult.result.valid());
}

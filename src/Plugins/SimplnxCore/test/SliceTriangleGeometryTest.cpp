/**
 * This file is auto generated from the original SimplnxCore/SliceTriangleGeometryFilter
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
 * When you start working on this unit test remove "[SliceTriangleGeometryFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "SimplnxCore/Filters/SliceTriangleGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

using namespace nx::core;

TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: Valid Filter Execution", "[SimplnxCore][SliceTriangleGeometryFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  SliceTriangleGeometryFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceDirection_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zstart_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_Zend_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceResolution_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_HaveRegionIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_TriangleGeometryDataPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_RegionIdArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_OutputEdgeGeometryPath_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_EdgeAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceIdArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(SliceTriangleGeometryFilter::k_SliceAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SimplnxCore::SliceTriangleGeometryFilter: InValid Filter Execution")
//{
//
// }

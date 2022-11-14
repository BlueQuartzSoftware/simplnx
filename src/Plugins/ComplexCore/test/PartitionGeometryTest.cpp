/**
 * This file is auto generated from the original Plugins/PartitionGeometryFilter
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
 * When you start working on this unit test remove "[PartitionGeometryFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/PartitionGeometryFilter.hpp"

using namespace complex;

TEST_CASE("Plugins::PartitionGeometryFilter: Instantiation and Parameter Check", "[Plugins][PartitionGeometryFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  PartitionGeometryFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(PartitionGeometryFilter::k_PartitioningMode_Key, std::make_any<ChoicesParameter::ValueType>(1234356));
  args.insertOrAssign(PartitionGeometryFilter::k_StartingPartitionID_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(PartitionGeometryFilter::k_OutOfBoundsValue_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(PartitionGeometryFilter::k_NumberOfPartitionsPerAxis_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3)));
  args.insertOrAssign(PartitionGeometryFilter::k_PartitioningSchemeOrigin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(PartitionGeometryFilter::k_LengthPerPartition_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(PartitionGeometryFilter::k_LowerLeftCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(PartitionGeometryFilter::k_UpperRightCoord_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(PartitionGeometryFilter::k_AttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(PartitionGeometryFilter::k_PSGeometry_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(PartitionGeometryFilter::k_PSGeometryAMName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(PartitionGeometryFilter::k_PSGeometryDataName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(PartitionGeometryFilter::k_GeometryToPartition_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(PartitionGeometryFilter::k_PartitionIdsArrayName_Key, std::make_any<std::string>(""));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("Plugins::PartitionGeometryFilter: Valid filter execution")
//{
//
//}

// TEST_CASE("Plugins::PartitionGeometryFilter: InValid filter execution")
//{
//
//}

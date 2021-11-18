/**
 * This file is auto generated from the original Core/CreateGeometry
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
 * When you start working on this unit test remove "[CreateGeometry][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/CreateGeometry.hpp"

using namespace complex;

TEST_CASE("Core::CreateGeometry: Instantiation and Parameter Check", "[Core][CreateGeometry][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateGeometry filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateGeometry::k_TreatWarningsAsErrors_Key, std::make_any<bool>(false));
  args.insertOrAssign(CreateGeometry::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(CreateGeometry::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3)));
  args.insertOrAssign(CreateGeometry::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(CreateGeometry::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(CreateGeometry::k_ImageCellAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_XBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_YBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_ZBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_RectGridCellAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath0_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName0_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath1_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedEdgeListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName1_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_EdgeAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath2_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedTriListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName2_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_FaceAttributeMatrixName0_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath3_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedQuadListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName3_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_FaceAttributeMatrixName1_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath4_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedTetListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName4_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_TetCellAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedVertexListArrayPath5_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_SharedHexListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_VertexAttributeMatrixName5_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateGeometry::k_HexCellAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("Core::CreateGeometry: Valid filter execution")
//{
//
//}

// TEST_CASE("Core::CreateGeometry: InValid filter execution")
//{
//
//}

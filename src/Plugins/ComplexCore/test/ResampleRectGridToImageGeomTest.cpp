#include <catch2/catch.hpp>

#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ResampleRectGridToImageGeomFilter.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::ResampleRectGridToImageGeomFilter: Valid Filter Execution", "[ComplexCore][ResampleRectGridToImageGeomFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3)));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrix_Key, std::make_any<std::string>(k_CellData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
}

// TEST_CASE("ComplexCore::ResampleRectGridToImageGeomFilter: InValid Filter Execution")
// {
//
// }

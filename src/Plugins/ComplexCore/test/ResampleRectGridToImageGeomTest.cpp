#include <catch2/catch.hpp>

#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ResampleRectGridToImageGeomFilter.hpp"

using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const DataPath k_RectGridGeomPath({"RectGridGeom"});
const DataPath k_RectGridCellDataPath = k_RectGridGeomPath.createChildPath(k_CellData);
const DataPath k_RectGridCellFeatureArrayPath = k_RectGridGeomPath.createChildPath(k_CellFeatureData).createChildPath(k_AvgQuats);

// const DataPath k_ImageGeomBiggerExemplar({"ImageGeom [Bigger]"});
// const DataPath k_ImageGeomSmallerExemplar({"ImageGeom [Smaller]"});
const DataPath k_ImageGeomComputed({"ImageGeom [Computed]"});
} // namespace

TEST_CASE("ComplexCore::ResampleRectGridToImageGeomFilter: Valid Filter Execution", "[ComplexCore][ResampleRectGridToImageGeomFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                          k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds)}));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrix_Key, std::make_any<std::string>(k_CellData));

  std::string exemplarImageGeomName;

  SECTION("Resample Smaller")
  {
    exemplarImageGeomName = "ImageGeom [Smaller]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Resample Bigger")
  {
    exemplarImageGeomName = "ImageGeom [Bigger]";
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 20)));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareExemplarToGeneratedData(dataStructure, dataStructure, k_ImageGeomComputed.createChildPath(k_CellData), exemplarImageGeomName);
}

TEST_CASE("ComplexCore::ResampleRectGridToImageGeomFilter: InValid Filter Execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeomCellAttributeMatrix_Key, std::make_any<std::string>(k_CellData));

  SECTION("Missing Selected Arrays")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Mismatching Selected Array Tuples")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(
                            MultiArraySelectionParameter::ValueType{k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles),
                                                                    k_RectGridCellDataPath.createChildPath(k_FeatureIds), k_RectGridCellFeatureArrayPath}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(3, 5)));
  }
  SECTION("Invalid Dimension Value")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                            k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds)}));
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_Dimensions_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{5, 5, -1}));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}

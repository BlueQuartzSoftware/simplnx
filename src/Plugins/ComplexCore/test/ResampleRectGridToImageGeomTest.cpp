#include <catch2/catch.hpp>

#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/StringUtilities.hpp"

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

const DataPath k_ImageGeomComputed({"ImageGeom [Computed]"});

const std::string k_NeighborListArrayName = "NeighborList";
const std::string k_StringArrayName = "StringArray";
} // namespace

TEST_CASE("ComplexCore::ResampleRectGridToImageGeomFilter: Valid Filter Execution", "[ComplexCore][ResampleRectGridToImageGeomFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz",
                                                             "6_6_resample_rect_grid_to_image_geom.dream3d", complex::unit_test::k_BinaryTestOutputDir);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ResampleRectGridToImageGeomFilter filter;
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_resample_rect_grid_to_image_geom.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create a neighbor list and string array for testing selected arrays with all IArray types
  {
    const auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(k_RectGridCellDataPath);
    const usize numTuples = cellDataAM.getNumTuples();

    std::vector<std::string> stringArrayValues(numTuples);
    auto* neighborList = NeighborList<float32>::Create(dataStructure, k_NeighborListArrayName, numTuples, cellDataAM.getId());
    neighborList->resizeTotalElements(numTuples);
    for(usize i = 0; i < numTuples; ++i)
    {
      const float32 factor = static_cast<float32>(i) * 0.001;
      std::vector<float32> list = {factor * 117, factor * 875, factor * 1035, factor * 3905, factor * 4214};
      neighborList->setList(i, std::make_shared<std::vector<float32>>(list));
      stringArrayValues[i] = "String_" + StringUtilities::number(factor);
    }

    const auto* stringArray = StringArray::CreateWithValues(dataStructure, k_StringArrayName, stringArrayValues, cellDataAM.getId());
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_RectilinearGridPath_Key, std::make_any<DataPath>(k_RectGridGeomPath));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomComputed));
  args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{
                          k_RectGridCellDataPath.createChildPath(k_ConfidenceIndex), k_RectGridCellDataPath.createChildPath(k_EulerAngles), k_RectGridCellDataPath.createChildPath(k_FeatureIds),
                          k_RectGridCellDataPath.createChildPath(k_NeighborListArrayName), k_RectGridCellDataPath.createChildPath(k_StringArrayName)}));
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
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_resample_rect_grid_to_image_geom.tar.gz",
                                                             "6_6_resample_rect_grid_to_image_geom.dream3d", complex::unit_test::k_BinaryTestOutputDir);
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
  SECTION("Invalid Cell Level Selected Array")
  {
    args.insertOrAssign(ResampleRectGridToImageGeomFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_RectGridCellFeatureArrayPath}));
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

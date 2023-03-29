#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AppendImageGeometryZSliceFilter.hpp"
#include "ComplexCore/Filters/CropImageGeometry.hpp"
#include "ComplexCore/Filters/RenameDataObject.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;
using namespace complex::UnitTest;

namespace
{
const DataPath k_CroppedBottomHalfPath({"CroppedBottomHalf"});
const DataPath k_CroppedTopHalfPath({"CroppedTopHalf"});
const DataPath k_AppendedGeometryPath({"AppendedGeometry"});
const DataPath k_InvalidTestGeometryPath1({"Image2dDataContainer"});
const DataPath k_InvalidTestGeometryPath2({"Resampled_2D_ImageGeom"});
const DataPath k_InvalidTestGeometryPath3({"Resampled_3D_ImageGeom"});
const DataPath k_SmallIN100Path({k_SmallIN100});
} // namespace

TEST_CASE("ComplexCore::AppendImageGeometryZSliceFilter: Valid Filter Execution", "[ComplexCore][AppendImageGeometryZSliceFilter]")
{
  // Read in starting/exemplar image geometry
  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  // crop input geometry down to bottom half & save as new geometry
  {
    CropImageGeometry filter;
    Arguments args;

    args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 0}));
    args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{188, 200, 50}));
    args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_CroppedBottomHalfPath));
    args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  // crop input geometry down to top half & save as new geometry
  {
    CropImageGeometry filter;
    Arguments args;

    args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 51}));
    args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{188, 200, 116}));
    args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(true));
    args.insert(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insert(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
    args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insert(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }
  // Append the 2 cropped geometries together and compare with the original input geometry
  {
    AppendImageGeometryZSliceFilter filter;
    Arguments args;

    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometry_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(k_CroppedBottomHalfPath));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_CheckResolution_Key, std::make_any<bool>(true));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_NewGeometry_Key, std::make_any<DataPath>(k_AppendedGeometryPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    // rename the cell data attribute matrix for easier comparison with exemplar data
    RenameDataObject renameFilter;
    Arguments renameArgs;
    renameArgs.insert(RenameDataObject::k_NewName_Key, std::make_any<std::string>(k_CellData));
    renameArgs.insert(RenameDataObject::k_DataObject_Key, std::make_any<DataPath>(k_AppendedGeometryPath.createChildPath(ImageGeom::k_CellDataName)));
    auto renameResult = renameFilter.execute(dataStructure, renameArgs);
    COMPLEX_RESULT_REQUIRE_VALID(renameResult.result)

    CompareExemplarToGeneratedData(dataStructure, dataStructure, k_AppendedGeometryPath.createChildPath(k_CellData), k_DataContainer);
  }
}

TEST_CASE("ComplexCore::AppendImageGeometryZSliceFilter: InValid Filter Execution", "[ComplexCore][AppendImageGeometryZSliceFilter]")
{
  // Read in test image geometries
  auto geomFilePath = fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(geomFilePath);

  AppendImageGeometryZSliceFilter filter;
  Arguments args;
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_CheckResolution_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_SaveAsNewGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(AppendImageGeometryZSliceFilter::k_NewGeometry_Key, std::make_any<DataPath>(k_AppendedGeometryPath));

  SECTION("Mismatching Dimensions")
  {
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometry_Key, std::make_any<DataPath>(k_InvalidTestGeometryPath3));
    args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(DataPath({k_SmallIN100})));
  }
  SECTION("Mismatching Spacing")
  {
    // crop one of the invalid test geometries to get a second geometry with the same dimensions but different spacing
    {
      CropImageGeometry cropFilter;
      Arguments cropArgs;

      cropArgs.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{0, 0, 0}));
      cropArgs.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(std::vector<uint64>{10, 10, 0}));
      cropArgs.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(true));
      cropArgs.insert(CropImageGeometry::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_InvalidTestGeometryPath2));
      cropArgs.insert(CropImageGeometry::k_CreatedImageGeometry_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
      cropArgs.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(false));
      cropArgs.insert(CropImageGeometry::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
      cropArgs.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

      auto cropResult = cropFilter.execute(dataStructure, cropArgs);
      COMPLEX_RESULT_REQUIRE_VALID(cropResult.result)
    }
    {
      args.insertOrAssign(AppendImageGeometryZSliceFilter::k_InputGeometry_Key, std::make_any<DataPath>(k_InvalidTestGeometryPath1));
      args.insertOrAssign(AppendImageGeometryZSliceFilter::k_DestinationGeometry_Key, std::make_any<DataPath>(k_CroppedTopHalfPath));
    }
  }

  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}

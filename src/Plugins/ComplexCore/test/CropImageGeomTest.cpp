#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CropImageGeometry.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("CropImageGeometry(Instantiate)", "[ComplexCore][CropImageGeometry]")
{
  static constexpr int32 k_MinX = 0;
  static constexpr int32 k_MinY = 0;
  static constexpr int32 k_MinZ = 0;
  static constexpr int32 k_MaxX = 0;
  static constexpr int32 k_MaxY = 0;
  static constexpr int32 k_MaxZ = 0;

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  std::vector<DataPath> k_VoxelArrays = {};
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  static constexpr StringLiteral k_NewFeaturesName = "New Feature IDs";

  CropImageGeometry filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometry::k_MinX_Key, std::make_any<int32>(k_MinX));
  args.insert(CropImageGeometry::k_MinY_Key, std::make_any<int32>(k_MinY));
  args.insert(CropImageGeometry::k_MinZ_Key, std::make_any<int32>(k_MinZ));
  args.insert(CropImageGeometry::k_MaxX_Key, std::make_any<int32>(k_MaxX));
  args.insert(CropImageGeometry::k_MaxY_Key, std::make_any<int32>(k_MaxY));
  args.insert(CropImageGeometry::k_MaxZ_Key, std::make_any<int32>(k_MaxZ));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_NewImageGeom_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_VoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometry::k_NewFeaturesName_Key, std::make_any<std::string>(k_NewFeaturesName));

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CropImageGeometry.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
usize convertPositionToOffset(const SizeVec3& position, const IDataStore::ShapeType& dimensions)
{
  usize xOffset = position.getX();
  usize yOffset = position.getY() * dimensions[0];
  usize zOffset = position.getZ() * dimensions[0] * dimensions[1];
  return xOffset + yOffset + zOffset;
}

void compareDataValues(const Int32Array& oldDataArray, const Int32Array& newDataArray)
{
  auto oldDimensions = oldDataArray.getDataStoreRef().getTupleShape();
  auto newDimensions = newDataArray.getDataStoreRef().getTupleShape();

  for(usize z = 0; z < oldDimensions[2] && z < newDimensions[2]; z++)
  {
    for(usize y = 0; y < oldDimensions[1] && y < newDimensions[1]; y++)
    {
      for(usize x = 0; x < oldDimensions[0] && x < newDimensions[0]; x++)
      {
        SizeVec3 position{x, y, z};
        auto newOffset = convertPositionToOffset(position, newDimensions);
        auto oldOffset = convertPositionToOffset(position, oldDimensions);

        auto oldData = oldDataArray[oldOffset];
        auto newData = newDataArray[newOffset];
        REQUIRE(oldData == newData);
      }
    }
  }
}
} // namespace

TEST_CASE("CropImageGeometry(Instantiate)", "[ComplexCore][CropImageGeometry]")
{
  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{0, 0, 0};

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

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
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

TEST_CASE("CropImageGeometry(Valid Parameters)", "[ComplexCore][CropImageGeometry]")
{
  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{2, 3, 4};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  const DataPath k_PhasesPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"});
  std::vector<DataPath> k_VoxelArrays = {k_PhasesPath};
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});
  static constexpr StringLiteral k_NewFeaturesName = "New Feature IDs";

  CropImageGeometry filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_NewImageGeom_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_VoxelArrays_Key, std::make_any<std::vector<DataPath>>(k_VoxelArrays));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometry::k_NewFeaturesName_Key, std::make_any<std::string>(k_NewFeaturesName));

  const auto oldDimensions = ds.getDataRefAs<ImageGeom>(k_ImageGeomPath).getDimensions();

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  auto& newImageGeom = ds.getDataRefAs<ImageGeom>(k_NewImageGeomPath);
  auto newDimensions = newImageGeom.getDimensions();

  for(usize i = 0; i < 3; i++)
  {
    REQUIRE(newDimensions[i] == (k_MaxVector[i] - k_MinVector[i] + 1));
  }

  const usize targetSize = (newDimensions[0]) * (newDimensions[1]) * (newDimensions[2]);
  const auto& oldDataArray = ds.getDataRefAs<Int32Array>(k_PhasesPath);
  const auto& newDataArray = ds.getDataRefAs<Int32Array>(k_NewImageGeomPath.createChildPath(k_NewFeaturesName).createChildPath("Phases"));
  const auto dataSize = newDataArray.getSize();
  REQUIRE(dataSize == targetSize);

  compareDataValues(oldDataArray, newDataArray);
}

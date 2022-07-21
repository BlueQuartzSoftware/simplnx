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

DataStructure CreateDataStructure()
{
  DataStructure dataGraph;
  DataGroup* topLevelGroup = DataGroup::Create(dataGraph, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataGraph, Constants::k_EbsdScanData, topLevelGroup->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, Constants::k_ImageGeometry, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  auto* cellData = AttributeMatrix::Create(dataGraph, ImageGeom::k_CellDataName, imageGeom->getId());
  auto imageDimsArray = imageGeomDims.toArray();
  AttributeMatrix::ShapeType cellDataDims{imageDimsArray.crbegin(), imageDimsArray.crend()};
  cellData->setShape(cellDataDims);
  imageGeom->setCellData(*cellData);

  Int32Array* phases_data = UnitTest::CreateTestDataArray<int32>(dataGraph, "Phases", cellDataDims, {1}, cellData->getId());

  return dataGraph;
}
} // namespace

TEST_CASE("ComplexCore::CropImageGeometry(Instantiate)", "[ComplexCore][CropImageGeometry]")
{
  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{0, 0, 0};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometry filter;
  DataStructure ds = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_NewImageGeom_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CropImageGeometry(Valid Parameters)", "[ComplexCore][CropImageGeometry]")
{
  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{2, 3, 4};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  const DataPath k_NewImageGeomPath({Constants::k_SmallIN100, "New Image Geom"});
  static constexpr bool k_RenumberFeatures = false;
  const DataPath k_FeatureIdsPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds});

  CropImageGeometry filter;
  DataStructure ds = CreateDataStructure();
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_NewImageGeom_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));

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
  const auto& oldDataArray = ds.getDataRefAs<Int32Array>(k_ImageGeomPath.createChildPath(IGridGeometry::k_CellDataName).createChildPath("Phases"));
  const auto& newDataArray = ds.getDataRefAs<Int32Array>(k_NewImageGeomPath.createChildPath(IGridGeometry::k_CellDataName).createChildPath("Phases"));
  const auto dataSize = newDataArray.getSize();
  REQUIRE(dataSize == targetSize);

  compareDataValues(oldDataArray, newDataArray);
}

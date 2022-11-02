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

template <typename T>
void compareCellDataValues(const DataArray<T>& oldDataArray, const DataArray<T>& newDataArray)
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
  args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}

TEST_CASE("ComplexCore::CropImageGeometry(Valid Parameters)", "[ComplexCore][CropImageGeometry]")
{
  const std::vector<uint64> k_MinVector{0, 0, 0};
  const std::vector<uint64> k_MaxVector{2, 3, 4};

  static constexpr bool k_UpdateOrigin = false;
  const DataPath k_ImageGeomPath({Constants::k_DataContainer});
  const DataPath k_NewImageGeomPath({"New Image Geom"});
  static constexpr bool k_RenumberFeatures = true;
  const DataPath k_FeatureIdsPath({Constants::k_DataContainer, Constants::k_CellData, Constants::k_FeatureIds});
  const DataPath k_CellFeatureAMPath({Constants::k_DataContainer, Constants::k_CellFeatureData});

  CropImageGeometry filter;
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1.dream3d", unit_test::k_TestDataSourceDir));
  DataStructure ds = UnitTest::LoadDataStructure(baseDataFilePath);
  Arguments args;

  args.insert(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(k_MinVector));
  args.insert(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(k_MaxVector));
  args.insert(CropImageGeometry::k_UpdateOrigin_Key, std::make_any<bool>(k_UpdateOrigin));
  args.insert(CropImageGeometry::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insert(CropImageGeometry::k_NewImageGeom_Key, std::make_any<DataPath>(k_NewImageGeomPath));
  args.insert(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(k_RenumberFeatures));
  args.insert(CropImageGeometry::k_FeatureIds_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insert(CropImageGeometry::k_CellFeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeatureAMPath));
  args.insert(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));

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
  const auto& oldPhasesDataArray = ds.getDataRefAs<Int32Array>(k_ImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_Phases));
  const auto& newPhasesDataArray = ds.getDataRefAs<Int32Array>(k_NewImageGeomPath.createChildPath(Constants::k_CellData).createChildPath(Constants::k_Phases));
  const auto dataSize = newPhasesDataArray.getSize();
  REQUIRE(dataSize == targetSize);
  compareCellDataValues<int32>(oldPhasesDataArray, newPhasesDataArray);

  {
    // Write out the DataStructure for later viewing/debugging
    Result<H5::FileWriter> ioResult = H5::FileWriter::CreateFile(fmt::format("{}/crop_image_geom_test.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(ioResult.value());
    herr_t err = ds.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }

  const auto& newCIDataArray = ds.getDataRefAs<Float32Array>(k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData).createChildPath(Constants::k_ConfidenceIndex));
  REQUIRE(std::fabs(newCIDataArray[0] - 0.0) < UnitTest::EPSILON);
  REQUIRE(std::fabs(newCIDataArray[1] - 0.8) < UnitTest::EPSILON);

  const auto& newIPFDataArray = ds.getDataRefAs<UInt8Array>(k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData).createChildPath(Constants::k_IPFColors));
  REQUIRE(newIPFDataArray[0] == 0);
  REQUIRE(newIPFDataArray[1] == 0);
  REQUIRE(newIPFDataArray[2] == 0);
  REQUIRE(newIPFDataArray[3] == 128);
  REQUIRE(newIPFDataArray[4] == 255);
  REQUIRE(newIPFDataArray[5] == 189);

  const auto& newEqDiamDataArray = ds.getDataRefAs<Float32Array>(k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData).createChildPath("EquivalentDiameters"));
  REQUIRE(std::fabs(newEqDiamDataArray[0] - 0.0) < UnitTest::EPSILON);
  REQUIRE(std::fabs(newEqDiamDataArray[1] - 16.661926) < UnitTest::EPSILON);

  const auto& newNumEltsDataArray = ds.getDataRefAs<Int32Array>(k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData).createChildPath("NumElements"));
  REQUIRE(newNumEltsDataArray[0] == 0);
  REQUIRE(newNumEltsDataArray[1] == 2422);

  const auto* newNeighborListArray = ds.getDataAs<INeighborList>(k_NewImageGeomPath.createChildPath(Constants::k_CellFeatureData).createChildPath("NeighborList"));
  REQUIRE(newNeighborListArray == nullptr);
}

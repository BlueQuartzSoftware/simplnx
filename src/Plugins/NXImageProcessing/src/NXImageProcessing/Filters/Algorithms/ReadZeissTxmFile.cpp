#include "ReadZeissTxmFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include "oless/oless.h"
#include "oless/pole.h"

using namespace nx::core;

namespace read_zeiss_txm
{

// -----------------------------------------------------------------------------
Result<ZeissTxmHeaderMetadata> ReadHeaderMetaData(const std::string& inputFilePath)
{
  const StoragePtrType storage = std::make_shared<POLE::Storage>(inputFilePath.c_str());
  storage->open();

  if(storage->result() != POLE::Storage::Ok)
  {
    return MakeErrorResult<ZeissTxmHeaderMetadata>(-33504, fmt::format("Could not open '{}' for reading", inputFilePath));
  }

  std::string path = "/ImageInfo/ImageWidth";
  const auto imageWidth = static_cast<size_t>(ReadValue<int32_t>(storage, path));

  path = "/ImageInfo/ImageHeight";
  const auto imageHeight = static_cast<size_t>(ReadValue<int32_t>(storage, path));

  path = "/ImageInfo/PixelSize";
  const auto pixelSize = ReadValue<float>(storage, path);

  path = "/ImageInfo/DataType";
  const auto dataType = static_cast<ZeissTxmDataType>(ReadValue<int32>(storage, path));

  path = "/ImageInfo/NoOfImages";
  const auto numImages = static_cast<size_t>(ReadValue<int32_t>(storage, path));

  // int numImageGroups = std::ceil(static_cast<float>(NoOfImages) / 100.0f);
  // fmt::print("Image Width: {}\n Image Height: {}\n PixelSize:{}\n DataType: {}\n NumImages: {}\n", imageWidth, imageHeight, pixelSize, to_underlying(dataType), numImages);
  storage->close();

  ZeissTxmHeaderMetadata metadata;
  metadata.dimensions = {imageWidth, imageHeight, numImages};
  metadata.spacing = {pixelSize, pixelSize, pixelSize};
  metadata.data_file_path = inputFilePath;
  metadata.data_type = dataType;
  return {metadata};
}

} // namespace read_zeiss_txm

using namespace read_zeiss_txm;

namespace
{

size_t ConvertDataTypeToByteCount(ZeissTxmDataType txmDataType)
{
  switch(txmDataType)
  {
  case ZeissTxmDataType::FLOAT_TYPE:
    return 4;
  case ZeissTxmDataType::INT16_TYPE:
    return 2;
  case ZeissTxmDataType::UCHAR_TYPE:
    return 1;
  }
  return 0;
}

template <typename T>
Result<> ReadImages(const ReadZeissTxmFileInputValues* inputValues, DataStructure& dataStructure, const ZeissTxmHeaderMetadata& metadata, const IFilter::MessageHandler& m_MessageHandler,
                    const std::atomic_bool& m_ShouldCancel)
{
  using DataArrayType = DataArray<T>;
  using DataStoreType = DataStore<T>;

  const DataPath dataArrayPath = inputValues->image_geometry_path.createChildPath(inputValues->cell_attribute_matrix_name).createChildPath(inputValues->density_array_name);
  auto& outputDataArray = dataStructure.getDataRefAs<DataArrayType>(dataArrayPath).getDataStoreRef();

  StoragePtrType storage = std::make_shared<POLE::Storage>(inputValues->txm_data_file.string().c_str());
  storage->open();

  if(storage->result() != POLE::Storage::Ok)
  {
    return MakeErrorResult(-33504, fmt::format("Could not open '{}' for reading", inputValues->txm_data_file.string()));
  }

  usize xStart = 0;
  usize xEnd = metadata.dimensions[0] - 1;
  usize yStart = 0;
  usize yEnd = metadata.dimensions[1] - 1;
  usize zStart = 0;
  usize zEnd = metadata.dimensions[2] - 1;
  if(inputValues->cropping_options.type == CropGeometryParameter::ValueType::TypeEnum::VoxelSubvolume)
  {
    if(inputValues->cropping_options.crop_x)
    {
      xStart = inputValues->cropping_options.x_bound_voxels[0];
      xEnd = inputValues->cropping_options.x_bound_voxels[1];
    }
    if(inputValues->cropping_options.crop_y)
    {
      yStart = inputValues->cropping_options.y_bound_voxels[0];
      yEnd = inputValues->cropping_options.y_bound_voxels[1];
    }
    if(inputValues->cropping_options.crop_z)
    {
      zStart = inputValues->cropping_options.z_bound_voxels[0];
      zEnd = inputValues->cropping_options.z_bound_voxels[1];
    }
  }
  else if(inputValues->cropping_options.type == CropGeometryParameter::ValueType::TypeEnum::PhysicalSubvolume)
  {
    // Create a temporary image geometry so that we can do some calculations on the source data
    DataStructure tmpDs;
    auto* srcImageGeomPtr = ImageGeom::Create(tmpDs, "srcImageGeom");
    if(auto result = srcImageGeomPtr->setOrigin(metadata.origin); result.invalid())
    {
      return result;
    }
    srcImageGeomPtr->setDimensions(metadata.dimensions);
    if(auto result = srcImageGeomPtr->setSpacing(metadata.spacing); result.invalid())
    {
      return result;
    }

    // Convert full source data dimensions to coordinates
    auto startCoords = srcImageGeomPtr->getCoordsf(xStart, yStart, zStart);
    auto endCoords = srcImageGeomPtr->getCoordsf(xEnd, yEnd, zEnd);

    // For each dimension, if cropping, use the physical cropping range.. otherwise if not cropping, use the full dimension coordinates
    FloatVec2Type xBoundPhysical = inputValues->cropping_options.crop_x ? inputValues->cropping_options.x_bound_physical : FloatVec2Type{startCoords[0], endCoords[0]};
    FloatVec2Type yBoundPhysical = inputValues->cropping_options.crop_y ? inputValues->cropping_options.y_bound_physical : FloatVec2Type{startCoords[1], endCoords[1]};
    FloatVec2Type zBoundPhysical = inputValues->cropping_options.crop_z ? inputValues->cropping_options.z_bound_physical : FloatVec2Type{startCoords[2], endCoords[2]};

    // Convert starting coordinates back to voxels
    std::optional<usize> result = srcImageGeomPtr->getIndex(xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]);
    if(!result.has_value())
    {
      storage->close();
      return MakeErrorResult(-33520,
                             fmt::format("Could not calculate the image geometry cell index that corresponds to the cropped geometry's starting physical bounds ({}, {}, {}).  This index is needed to "
                                         "determine which block of image geometry cells to crop.",
                                         xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]));
    }
    xStart = result.value() % metadata.dimensions[0];
    yStart = (result.value() / metadata.dimensions[0]) % metadata.dimensions[1];
    zStart = result.value() / (metadata.dimensions[0] * metadata.dimensions[1]);

    // Convert ending coordinates back to voxels
    result = srcImageGeomPtr->getIndex(xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]);
    if(!result.has_value())
    {
      storage->close();
      return MakeErrorResult(-33521,
                             fmt::format("Could not calculate the image geometry cell index that corresponds to the cropped geometry's ending physical bounds ({}, {}, {}).  This index is needed to "
                                         "determine which block of image geometry cells to crop.",
                                         xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]));
    }
    xEnd = result.value() % metadata.dimensions[0];
    yEnd = (result.value() / metadata.dimensions[0]) % metadata.dimensions[1];
    zEnd = result.value() / (metadata.dimensions[0] * metadata.dimensions[1]);
  }

  size_t totalPixels = metadata.dimensions[0] * metadata.dimensions[1];
  size_t totalBytes = totalPixels * ConvertDataTypeToByteCount(metadata.data_type);
  size_t readBytes = 0;
  std::vector<T> buffer(totalPixels);

  usize imageGroupIndex = zStart / 100 + 1;

  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->image_geometry_path);
  Vec3<usize> destDims = destImageGeom.getDimensions();
  for(usize z = zStart + 1; z <= zEnd + 1; z++)
  {
    if(m_ShouldCancel)
    {
      storage->close();
      return {};
    }
    std::stringstream pathStrm;
    pathStrm << "/ImageData" << imageGroupIndex << "/Image" << z;
    m_MessageHandler.sendInfoMessage(fmt::format("Constructing Image Path: {}", pathStrm.str()));

    StreamPtrType stream = std::make_shared<POLE::Stream>(storage.get(), pathStrm.str());
    if(!stream->fail())
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Reading Image: {}", pathStrm.str()));
      readBytes = stream->read(reinterpret_cast<unsigned char*>(buffer.data()), totalBytes);
      if(readBytes != totalBytes)
      {
        return MakeErrorResult(-33510, fmt::format("Not enough bytes were read: {} vs {}", readBytes, totalBytes));
      }

      for(usize y = yStart; y <= yEnd; y++)
      {
        usize srcTupleOffset = (metadata.dimensions[0] * y) + xStart;
        usize dstTupleOffset = (destDims[1] * destDims[0] * (z - 1 - zStart)) + (destDims[0] * (y - yStart));
        Result<> result = copy_from_array::CopyData(buffer, outputDataArray, dstTupleOffset, srcTupleOffset, xEnd - xStart + 1, 1);
        if(result.invalid())
        {
          return MakeErrorResult(-33511, fmt::format("Error copying data from buffer to output data array: {}", result.errors()[0].message));
        }
      }
    }
    else
    {
      return MakeErrorResult(-33512, fmt::format("Failed to read image from OLE Path '{}'", pathStrm.str()));
    }
    if(z % 100 == 0)
    {
      imageGroupIndex++;
    }
  }

  storage->close();
  return {};
}

} // namespace

// -----------------------------------------------------------------------------
ReadZeissTxmFile::ReadZeissTxmFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadZeissTxmFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadZeissTxmFile::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadZeissTxmFile::operator()() const
{

  Result<ZeissTxmHeaderMetadata> metadataResult = ReadHeaderMetaData(m_InputValues->txm_data_file.string());
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  ZeissTxmHeaderMetadata metaData = metadataResult.value();
  if(auto geometryResult = m_InputValues->geometry_overrides.apply(m_InputValues->txm_data_file, metaData.origin, metaData.spacing); geometryResult.invalid())
  {
    return geometryResult;
  }

  switch(metaData.data_type)
  {
  case ZeissTxmDataType::FLOAT_TYPE:
    return ReadImages<float32>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  case ZeissTxmDataType::INT16_TYPE:
    return ReadImages<uint16>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  case ZeissTxmDataType::UCHAR_TYPE:
    return ReadImages<uint8>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  default:;
  }
  return MakeErrorResult(-33520, fmt::format("Unsupported data type: {}", to_underlying(metaData.data_type)));
}

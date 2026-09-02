#include "ReadZeissTxmFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include "oless/oless.h"
#include "oless/pole.h"

using namespace nx::core;
namespace fs = std::filesystem;

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
  metadata.Dimensions = {imageWidth, imageHeight, numImages};
  metadata.Spacing = {pixelSize, pixelSize, pixelSize};
  metadata.DataFilePath = inputFilePath;
  metadata.DataType = dataType;
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

  const DataPath dataArrayPath = inputValues->ImageGeometryPath.createChildPath(inputValues->CellAttributeMatrixName).createChildPath(inputValues->DensityArrayName);
  auto& outputDataArray = dataStructure.getDataRefAs<DataArrayType>(dataArrayPath).getDataStoreRef();

  StoragePtrType storage = std::make_shared<POLE::Storage>(inputValues->TxmDataFile.string().c_str());
  storage->open();

  if(storage->result() != POLE::Storage::Ok)
  {
    return MakeErrorResult(-33504, fmt::format("Could not open '{}' for reading", inputValues->TxmDataFile.string()));
  }

  usize xStart = 0;
  usize xEnd = metadata.Dimensions[0] - 1;
  usize yStart = 0;
  usize yEnd = metadata.Dimensions[1] - 1;
  usize zStart = 0;
  usize zEnd = metadata.Dimensions[2] - 1;
  if(inputValues->CroppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::VoxelSubvolume)
  {
    if(inputValues->CroppingOptions.cropX)
    {
      xStart = inputValues->CroppingOptions.xBoundVoxels[0];
      xEnd = inputValues->CroppingOptions.xBoundVoxels[1];
    }
    if(inputValues->CroppingOptions.cropY)
    {
      yStart = inputValues->CroppingOptions.yBoundVoxels[0];
      yEnd = inputValues->CroppingOptions.yBoundVoxels[1];
    }
    if(inputValues->CroppingOptions.cropZ)
    {
      zStart = inputValues->CroppingOptions.zBoundVoxels[0];
      zEnd = inputValues->CroppingOptions.zBoundVoxels[1];
    }
  }
  else if(inputValues->CroppingOptions.type == CropGeometryParameter::ValueType::TypeEnum::PhysicalSubvolume)
  {
    // Create a temporary image geometry so that we can do some calculations on the source data
    DataStructure tmpDs;
    auto* srcImageGeom = ImageGeom::Create(tmpDs, "srcImageGeom");
    srcImageGeom->setOrigin(metadata.Origin);
    srcImageGeom->setDimensions(metadata.Dimensions);
    srcImageGeom->setSpacing(metadata.Spacing);

    // Convert full source data dimensions to coordinates
    auto startCoords = srcImageGeom->getCoordsf(xStart, yStart, zStart);
    auto endCoords = srcImageGeom->getCoordsf(xEnd, yEnd, zEnd);

    // For each dimension, if cropping, use the physical cropping range.. otherwise if not cropping, use the full dimension coordinates
    FloatVec2Type xBoundPhysical = inputValues->CroppingOptions.cropX ? inputValues->CroppingOptions.xBoundPhysical : FloatVec2Type{startCoords[0], endCoords[0]};
    FloatVec2Type yBoundPhysical = inputValues->CroppingOptions.cropY ? inputValues->CroppingOptions.yBoundPhysical : FloatVec2Type{startCoords[1], endCoords[1]};
    FloatVec2Type zBoundPhysical = inputValues->CroppingOptions.cropZ ? inputValues->CroppingOptions.zBoundPhysical : FloatVec2Type{startCoords[2], endCoords[2]};

    // Convert starting coordinates back to voxels
    std::optional<usize> result = srcImageGeom->getIndex(xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]);
    if(!result.has_value())
    {
      storage->close();
      return MakeErrorResult(-33520,
                             fmt::format("Could not calculate the image geometry cell index that corresponds to the cropped geometry's starting physical bounds ({}, {}, {}).  This index is needed to "
                                         "determine which block of image geometry cells to crop.",
                                         xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]));
    }
    xStart = result.value() % metadata.Dimensions[0];
    yStart = (result.value() / metadata.Dimensions[0]) % metadata.Dimensions[1];
    zStart = result.value() / (metadata.Dimensions[0] * metadata.Dimensions[1]);

    // Convert ending coordinates back to voxels
    result = srcImageGeom->getIndex(xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]);
    if(!result.has_value())
    {
      storage->close();
      return MakeErrorResult(-33521,
                             fmt::format("Could not calculate the image geometry cell index that corresponds to the cropped geometry's ending physical bounds ({}, {}, {}).  This index is needed to "
                                         "determine which block of image geometry cells to crop.",
                                         xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]));
    }
    xEnd = result.value() % metadata.Dimensions[0];
    yEnd = (result.value() / metadata.Dimensions[0]) % metadata.Dimensions[1];
    zEnd = result.value() / (metadata.Dimensions[0] * metadata.Dimensions[1]);
  }

  size_t totalPixels = metadata.Dimensions[0] * metadata.Dimensions[1];
  size_t totalBytes = totalPixels * ConvertDataTypeToByteCount(metadata.DataType);
  size_t readBytes = 0;
  std::vector<T> buffer(totalPixels);

  usize imageGroupIndex = zStart / 100 + 1;

  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->ImageGeometryPath);
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
        usize srcTupleOffset = (metadata.Dimensions[0] * y) + xStart;
        usize dstTupleOffset = (destDims[1] * destDims[0] * (z - 1 - zStart)) + (destDims[0] * (y - yStart));
        Result<> result = CopyFromArray::CopyData(buffer, outputDataArray, dstTupleOffset, srcTupleOffset, xEnd - xStart + 1, 1);
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
ReadZeissTxmFile::~ReadZeissTxmFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadZeissTxmFile::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadZeissTxmFile::operator()() const
{

  Result<ZeissTxmHeaderMetadata> metadataResult = ReadHeaderMetaData(m_InputValues->TxmDataFile.string());
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  ZeissTxmHeaderMetadata metaData = metadataResult.value();

  switch(metaData.DataType)
  {
  case ZeissTxmDataType::FLOAT_TYPE:
    return ReadImages<float32>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  case ZeissTxmDataType::INT16_TYPE:
    return ReadImages<uint16>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  case ZeissTxmDataType::UCHAR_TYPE:
    return ReadImages<uint8>(m_InputValues, m_DataStructure, metaData, m_MessageHandler, m_ShouldCancel);
  default:;
  }
  return MakeErrorResult(-33520, fmt::format("Unsupported data type: {}", to_underlying(metaData.DataType)));
}

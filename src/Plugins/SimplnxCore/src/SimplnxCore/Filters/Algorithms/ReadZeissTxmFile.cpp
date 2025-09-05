#include "ReadZeissTxmFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
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
    MakeErrorResult(-33504, fmt::format("Could not open '{}' for reading", inputFilePath));
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
Result<> ReadImages(const ReadZeissTxmFileInputValues* inputValues, DataStructure& dataStructure, const ZeissTxmHeaderMetadata& metadata, const IFilter::MessageHandler& m_MessageHandler)
{
  using DataArrayType = DataArray<T>;

  const DataPath dataArrayPath = inputValues->ImageGeometryPath.createChildPath(inputValues->CellAttributeMatrixName).createChildPath(inputValues->DensityArrayName);
  auto& outputDataArray = dataStructure.getDataRefAs<DataArrayType>(dataArrayPath).getDataStoreRef();

  StoragePtrType storage = std::make_shared<POLE::Storage>(inputValues->TxmDataFile.string().c_str());
  storage->open();

  if(storage->result() != POLE::Storage::Ok)
  {
    MakeErrorResult(-33504, fmt::format("Could not open '{}' for reading", inputValues->TxmDataFile.string()));
  }

  size_t totalPixels = metadata.Dimensions[0] * metadata.Dimensions[1];
  usize elementsRead = totalPixels;
  size_t totalBytes = totalPixels * ConvertDataTypeToByteCount(metadata.DataType);
  size_t readBytes = 0;
  std::vector<T> buffer(totalPixels);

  usize elementCounter = 0;

  int32 startSlice = 1;
  int32 endSlice = metadata.Dimensions[2];
  if(inputValues->UseSubVolume)
  {
    startSlice = inputValues->SubVolumeStartSlice;
    endSlice = inputValues->SubVolumeEndSlice;
  }

  int32 imageGroupIndex = (startSlice - 1) / 100 + 1;

  for(int32_t i = startSlice; i <= endSlice; i++)
  {
    std::stringstream pathStrm;
    pathStrm << "/ImageData" << imageGroupIndex << "/Image" << i;
    // m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Constructing Image Path: {}", pathStrm.str())});

    StreamPtrType stream = std::make_shared<POLE::Stream>(storage.get(), pathStrm.str());
    if(!stream->fail())
    {
      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Reading Image: {}", pathStrm.str())});
      readBytes = stream->read(reinterpret_cast<unsigned char*>(buffer.data()), totalBytes);
      if(readBytes != totalBytes)
      {
        MakeErrorResult(-33510, fmt::format("Not enough bytes were read: {} vs {}", readBytes, totalBytes));
      }
      for(usize e = 0; e < elementsRead; e++)
      {
        outputDataArray[e + elementCounter] = buffer[e];
      }

      elementCounter += elementsRead;
    }
    else
    {
      return MakeErrorResult(-33511, fmt::format("Failed to read image from OLE Path '{}'", pathStrm.str()));
    }
    if(i % 100 == 0)
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
    return ReadImages<float32>(m_InputValues, m_DataStructure, metaData, m_MessageHandler);
  case ZeissTxmDataType::INT16_TYPE:
    return ReadImages<uint16>(m_InputValues, m_DataStructure, metaData, m_MessageHandler);
  case ZeissTxmDataType::UCHAR_TYPE:
    return ReadImages<uint8>(m_InputValues, m_DataStructure, metaData, m_MessageHandler);
  default:;
  }
  return MakeErrorResult(-33520, fmt::format("Unsupported data type: {}", to_underlying(metaData.DataType)));
}

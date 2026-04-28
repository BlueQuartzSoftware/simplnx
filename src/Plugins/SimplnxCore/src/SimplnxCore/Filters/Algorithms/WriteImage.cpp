#include "WriteImage.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cstring>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// Writes a single element of type T to a raw byte buffer at the given byte offset
// without violating strict aliasing rules.
template <typename T>
void WriteElementAs(uint8* data, usize byteOffset, T value)
{
  std::memcpy(data + byteOffset, &value, sizeof(T));
}

/**
 * @brief Functor that extracts a single 2D slice from a typed DataStore
 * into a raw byte buffer suitable for IImageIO::writePixelData().
 */
struct ExtractSliceFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray& dataArray, std::vector<uint8>& buffer, usize sliceIndex, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize nComp)
  {
    const auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    uint8* bufferData = buffer.data();

    if(planeIndex == 0) // XY plane — iterate over Z, slice width=X, slice height=Y
    {
      usize z = sliceIndex;
      for(usize y = 0; y < dimY; ++y)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          usize srcIndex = (z * dimY * dimX + y * dimX + x) * nComp;
          usize dstIndex = (y * dimX + x) * nComp;
          for(usize c = 0; c < nComp; ++c)
          {
            WriteElementAs<T>(bufferData, (dstIndex + c) * sizeof(T), dataStore.getValue(srcIndex + c));
          }
        }
      }
    }
    else if(planeIndex == 1) // XZ plane — iterate over Y, slice width=X, slice height=Z
    {
      usize y = sliceIndex;
      for(usize z = 0; z < dimZ; ++z)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          usize srcIndex = (z * dimY * dimX + y * dimX + x) * nComp;
          usize dstIndex = (z * dimX + x) * nComp;
          for(usize c = 0; c < nComp; ++c)
          {
            WriteElementAs<T>(bufferData, (dstIndex + c) * sizeof(T), dataStore.getValue(srcIndex + c));
          }
        }
      }
    }
    else if(planeIndex == 2) // YZ plane — iterate over X, slice width=Y, slice height=Z
    {
      usize x = sliceIndex;
      for(usize z = 0; z < dimZ; ++z)
      {
        for(usize y = 0; y < dimY; ++y)
        {
          usize srcIndex = (z * dimY * dimX + y * dimX + x) * nComp;
          usize dstIndex = (z * dimY + y) * nComp;
          for(usize c = 0; c < nComp; ++c)
          {
            WriteElementAs<T>(bufferData, (dstIndex + c) * sizeof(T), dataStore.getValue(srcIndex + c));
          }
        }
      }
    }

    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
WriteImage::WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteImage::~WriteImage() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteImage::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues.imageGeometryPath);
  const auto& imageArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.imageDataArrayPath);

  // ImageGeometry dimensions are stored fastest-to-slowest (X, Y, Z)
  SizeVec3 dims = imageGeom.getDimensions();
  usize dimX = dims[0];
  usize dimY = dims[1];
  usize dimZ = dims[2];

  usize nComp = imageArray.getNumberOfComponents();
  DataType dataType = imageArray.getDataType();
  usize bytesPerComponent = imageArray.getIDataStoreRef().getTypeSize();

  usize sliceCount = 0;
  usize sliceW = 0;
  usize sliceH = 0;

  switch(m_InputValues.planeIndex)
  {
  case 0: // XY
    sliceCount = dimZ;
    sliceW = dimX;
    sliceH = dimY;
    break;
  case 1: // XZ
    sliceCount = dimY;
    sliceW = dimX;
    sliceH = dimZ;
    break;
  case 2: // YZ
    sliceCount = dimX;
    sliceW = dimY;
    sliceH = dimZ;
    break;
  default:
    return MakeErrorResult(-27000, fmt::format("Invalid plane index: {}", m_InputValues.planeIndex));
  }

  auto imageIOResult = CreateImageIO(m_InputValues.outputFilePath);
  if(imageIOResult.invalid())
  {
    return ConvertResult(std::move(imageIOResult));
  }
  const auto& imageIO = imageIOResult.value();

  fs::path parentDir = fs::absolute(m_InputValues.outputFilePath).parent_path();
  if(!fs::exists(parentDir))
  {
    if(!fs::create_directories(parentDir))
    {
      return MakeErrorResult(-27001, fmt::format("Error creating output directory '{}'", parentDir.string()));
    }
  }

  fs::path stem = m_InputValues.outputFilePath.stem();
  fs::path ext = m_InputValues.outputFilePath.extension();
  fs::path parent = fs::absolute(m_InputValues.outputFilePath).parent_path();

  usize sliceBufferSize = sliceW * sliceH * nComp * bytesPerComponent;
  std::vector<uint8> sliceBuffer(sliceBufferSize);

  // ImageMetadata is invariant across slices for a single volume; hoist out of the loop.
  ImageMetadata metadata;
  metadata.width = sliceW;
  metadata.height = sliceH;
  metadata.numComponents = nComp;
  metadata.dataType = dataType;
  metadata.numPages = 1;

  for(usize slice = 0; slice < sliceCount; ++slice)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing slice {}/{}", slice + 1, sliceCount));

    // No need to zero sliceBuffer — ExtractSliceFunctor writes every byte of the slice plane.
    auto extractResult = ExecuteDataFunction(ExtractSliceFunctor{}, dataType, imageArray, sliceBuffer, slice, m_InputValues.planeIndex, dimX, dimY, dimZ, nComp);
    if(extractResult.invalid())
    {
      return extractResult;
    }

    std::string indexStr = CreateIndexString(slice + m_InputValues.indexOffset, static_cast<usize>(m_InputValues.totalIndexDigits), m_InputValues.leadingDigitCharacter);
    fs::path slicePath = parent / fmt::format("{}_{}{}", stem.string(), indexStr, ext.string());

    auto atomicFileResult = AtomicFile::Create(slicePath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    auto writeResult = imageIO->writePixelData(atomicFile.tempFilePath(), sliceBuffer, metadata);
    if(writeResult.invalid())
    {
      return writeResult;
    }

    auto commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  return {};
}

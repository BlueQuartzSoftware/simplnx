#include "ReadImage.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

using namespace nx::core;

namespace
{
// Cropping window inside the source image. If no cropping is used, the window covers the full image.
struct CropWindow
{
  usize srcWidth = 0;
  usize srcHeight = 0;
  usize dstWidth = 0;
  usize dstHeight = 0;
  usize xStart = 0;
  usize yStart = 0;
  usize numComponents = 1;
};

// Extracts a single element of type T from a raw byte buffer at the given byte offset
// without violating strict aliasing rules.
template <typename T>
T ReadElementAs(const uint8* data, usize byteOffset)
{
  T value;
  std::memcpy(&value, data + byteOffset, sizeof(T));
  return value;
}

struct CopyPixelDataFunctor
{
  template <typename T>
  Result<> operator()(IDataArray& dataArray, const std::vector<uint8>& buffer, const CropWindow& window)
  {
    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const uint8* bufferData = buffer.data();

    const usize nComps = window.numComponents;
    const usize srcWidth = window.srcWidth;
    const usize dstWidth = window.dstWidth;
    const usize xStart = window.xStart;
    const usize yStart = window.yStart;

    for(usize y = 0; y < window.dstHeight; y++)
    {
      const usize srcY = y + yStart;
      for(usize x = 0; x < window.dstWidth; x++)
      {
        const usize srcX = x + xStart;
        const usize srcIndex = (srcY * srcWidth + srcX) * nComps;
        const usize dstIndex = (y * dstWidth + x) * nComps;
        for(usize c = 0; c < nComps; c++)
        {
          dataStore[dstIndex + c] = ReadElementAs<T>(bufferData, (srcIndex + c) * sizeof(T));
        }
      }
    }
    return {};
  }
};

template <typename SrcT>
struct ConvertPixelDataFunctor
{
  template <typename DestT>
  Result<> operator()(IDataArray& dataArray, const std::vector<uint8>& buffer, const CropWindow& window)
  {
    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<DestT>>();
    const uint8* bufferData = buffer.data();

    // For integer source/dest types the saturation value is the type's max. For floating-point
    // we follow stb's HDR convention that pixel values lie in [0, 1] and saturate outside that
    // range. Without this the integer-max divisor produced near-zero output for any HDR float
    // input (e.g. 0.5 / float32::max() ≈ 0) and rendered every converted pixel black.
    constexpr double srcMax = std::is_floating_point_v<SrcT> ? 1.0 : static_cast<double>(std::numeric_limits<SrcT>::max());
    constexpr double destMax = std::is_floating_point_v<DestT> ? 1.0 : static_cast<double>(std::numeric_limits<DestT>::max());

    const usize nComps = window.numComponents;
    const usize srcWidth = window.srcWidth;
    const usize dstWidth = window.dstWidth;
    const usize xStart = window.xStart;
    const usize yStart = window.yStart;

    for(usize y = 0; y < window.dstHeight; y++)
    {
      const usize srcY = y + yStart;
      for(usize x = 0; x < window.dstWidth; x++)
      {
        const usize srcX = x + xStart;
        const usize srcIndex = (srcY * srcWidth + srcX) * nComps;
        const usize dstIndex = (y * dstWidth + x) * nComps;
        for(usize c = 0; c < nComps; c++)
        {
          const SrcT srcValue = ReadElementAs<SrcT>(bufferData, (srcIndex + c) * sizeof(SrcT));
          // Clamp into the source's saturation range before normalizing so HDR floats > 1.0 or
          // negative values do not wrap around through the destination's representable range.
          const double clampedSrc = std::clamp(static_cast<double>(srcValue), 0.0, srcMax);
          const double normalized = clampedSrc / srcMax;
          dataStore[dstIndex + c] = static_cast<DestT>(normalized * destMax);
        }
      }
    }
    return {};
  }
};

struct DispatchConversionFunctor
{
  template <typename SrcT>
  Result<> operator()(DataType destType, IDataArray& dataArray, const std::vector<uint8>& buffer, const CropWindow& window)
  {
    return ExecuteDataFunction(ConvertPixelDataFunctor<SrcT>{}, destType, dataArray, buffer, window);
  }
};
} // namespace

// -----------------------------------------------------------------------------
ReadImage::ReadImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadImage::~ReadImage() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadImage::operator()()
{
  const auto& inputFilePath = m_InputValues.inputFilePath;

  m_MessageHandler.sendInfoMessage(fmt::format("Reading image file: {}", inputFilePath.string()));

  auto imageIOResult = CreateImageIO(inputFilePath);
  if(imageIOResult.invalid())
  {
    return ConvertResult(std::move(imageIOResult));
  }
  auto& imageIO = imageIOResult.value();

  auto metadataResult = imageIO->readMetadata(inputFilePath);
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  const auto& metadata = metadataResult.value();

  usize bytesPerComp = GetDataTypeSize(metadata.dataType);
  usize bufferSize = metadata.width * metadata.height * metadata.numComponents * bytesPerComp;

  std::vector<uint8> tempBuffer(bufferSize);
  auto readResult = imageIO->readPixelData(inputFilePath, tempBuffer);
  if(readResult.invalid())
  {
    return readResult;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  auto& imageArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.imageDataArrayPath);
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues.imageGeometryPath);
  const SizeVec3 geomDims = imageGeom.getDimensions();

  CropWindow window;
  window.srcWidth = metadata.width;
  window.srcHeight = metadata.height;
  window.numComponents = metadata.numComponents;
  window.dstWidth = geomDims[0];
  window.dstHeight = geomDims[1];
  window.xStart = 0;
  window.yStart = 0;

  const auto& croppingOptions = m_InputValues.croppingOptions;
  const bool cropImage = croppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping;
  const bool crop2dImage = cropImage && (croppingOptions.cropX || croppingOptions.cropY);
  if(crop2dImage)
  {
    if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
    {
      if(croppingOptions.cropX)
      {
        window.xStart = static_cast<usize>(croppingOptions.xBoundVoxels[0]);
      }
      if(croppingOptions.cropY)
      {
        window.yStart = static_cast<usize>(croppingOptions.yBoundVoxels[0]);
      }
    }
    else // PhysicalSubvolume
    {
      // Convert physical coordinates to source voxel indices using the file's native origin/spacing.
      // The ImageGeom's origin/spacing may have been overridden in preflight, but cropping bounds are
      // interpreted against whatever origin/spacing was active when the crop filter ran. In the
      // Preprocessed case, overrides were applied before cropping, so we mirror them here.
      FloatVec3 srcOrigin = metadata.origin.value_or(FloatVec3{0.0f, 0.0f, 0.0f});
      FloatVec3 srcSpacing = metadata.spacing.value_or(FloatVec3{1.0f, 1.0f, 1.0f});
      if(m_InputValues.originSpacingProcessing == OriginSpacingProcessing::Preprocessed)
      {
        if(m_InputValues.changeSpacing)
        {
          srcSpacing = m_InputValues.spacing;
        }
        if(m_InputValues.changeOrigin)
        {
          srcOrigin = m_InputValues.origin;
          if(m_InputValues.centerOrigin)
          {
            srcOrigin[0] = -0.5f * srcSpacing[0] * static_cast<float32>(metadata.width);
            srcOrigin[1] = -0.5f * srcSpacing[1] * static_cast<float32>(metadata.height);
            srcOrigin[2] = 0.0f;
          }
        }
      }
      if(croppingOptions.cropX)
      {
        const float64 xMin = static_cast<float64>(croppingOptions.xBoundPhysical[0]);
        const int64 voxelX = static_cast<int64>((xMin - static_cast<float64>(srcOrigin[0])) / static_cast<float64>(srcSpacing[0]));
        if(voxelX < 0 || static_cast<usize>(voxelX) >= window.srcWidth)
        {
          return MakeErrorResult(-2002, fmt::format("Physical crop X minimum {} is outside the source image extent [{}, {}) given file origin {} and spacing {}", xMin, srcOrigin[0],
                                                    srcOrigin[0] + srcSpacing[0] * static_cast<float32>(window.srcWidth), srcOrigin[0], srcSpacing[0]));
        }
        window.xStart = static_cast<usize>(voxelX);
      }
      if(croppingOptions.cropY)
      {
        const float64 yMin = static_cast<float64>(croppingOptions.yBoundPhysical[0]);
        const int64 voxelY = static_cast<int64>((yMin - static_cast<float64>(srcOrigin[1])) / static_cast<float64>(srcSpacing[1]));
        if(voxelY < 0 || static_cast<usize>(voxelY) >= window.srcHeight)
        {
          return MakeErrorResult(-2003, fmt::format("Physical crop Y minimum {} is outside the source image extent [{}, {}) given file origin {} and spacing {}", yMin, srcOrigin[1],
                                                    srcOrigin[1] + srcSpacing[1] * static_cast<float32>(window.srcHeight), srcOrigin[1], srcSpacing[1]));
        }
        window.yStart = static_cast<usize>(voxelY);
      }
    }
  }

  if(window.xStart + window.dstWidth > window.srcWidth || window.yStart + window.dstHeight > window.srcHeight)
  {
    return MakeErrorResult(-2001, fmt::format("Crop window (start=[{},{}], size=[{},{}]) does not fit within the source image (size=[{},{}])", window.xStart, window.yStart, window.dstWidth,
                                              window.dstHeight, window.srcWidth, window.srcHeight));
  }

  DataType destType = imageArray.getDataType();
  DataType srcType = metadata.dataType;

  if(m_InputValues.changeDataType && srcType != destType)
  {
    m_MessageHandler.sendInfoMessage(fmt::format("Converting pixel data from {} to {}", DataTypeToString(srcType), DataTypeToString(destType)));
    auto convResult = ExecuteDataFunction(DispatchConversionFunctor{}, srcType, destType, imageArray, tempBuffer, window);
    if(convResult.invalid())
    {
      return convResult;
    }
  }
  else
  {
    auto copyResult = ExecuteDataFunction(CopyPixelDataFunctor{}, srcType, imageArray, tempBuffer, window);
    if(copyResult.invalid())
    {
      return copyResult;
    }
  }

  return {};
}

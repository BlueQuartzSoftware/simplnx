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
#include <memory>
#include <type_traits>

using namespace nx::core;

namespace
{
/**
 * @struct CropWindow
 * @brief Stores source crop bounds and destination image shape.
 */
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

/**
 * @brief Copies one typed element from raw decoder bytes.
 * @tparam T Specifies the decoded scalar type.
 * @param data Provides decoder bytes.
 * @param byteOffset Specifies the first byte to copy.
 * @return Decoded value.
 *
 * memcpy avoids alignment and aliasing violations.
 */
template <typename T>
T ReadElementAs(const uint8* data, usize byteOffset)
{
  T value;
  std::memcpy(&value, data + byteOffset, sizeof(T));
  return value;
}

/**
 * @struct CopyPixelDataFunctor
 * @brief Writes decoder bytes when source and destination types match.
 */
struct CopyPixelDataFunctor
{
  /**
   * @brief Copies one decoded row segment to the destination store.
   * @tparam T Specifies the shared source and destination scalar type.
   * @param dataArray Receives decoded values.
   * @param bytes Provides a complete number of typed values.
   * @param destinationOffset Specifies the first destination value.
   * @return Destination bulk-write result.
   */
  template <typename T>
  Result<> operator()(IDataArray& dataArray, std::span<const uint8> bytes, usize destinationOffset)
  {
    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize elementCount = bytes.size() / sizeof(T);
    auto values = std::make_unique<T[]>(elementCount);
    std::memcpy(values.get(), bytes.data(), bytes.size());
    return dataStore.copyFromBuffer(destinationOffset, nonstd::span<const T>(values.get(), elementCount));
  }
};

/**
 * @struct ConvertPixelDataFunctor
 * @brief Converts decoder bytes through normalized scalar values.
 * @tparam SrcT Specifies the decoder scalar type.
 */
template <typename SrcT>
struct ConvertPixelDataFunctor
{
  /**
   * @brief Converts and writes one decoded row segment.
   * @tparam DestT Specifies the destination scalar type.
   * @param dataArray Receives converted values.
   * @param bytes Provides a complete number of source values.
   * @param destinationOffset Specifies the first destination value.
   * @return Destination bulk-write result.
   */
  template <typename DestT>
  Result<> operator()(IDataArray& dataArray, std::span<const uint8> bytes, usize destinationOffset)
  {
    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<DestT>>();

    // Integer types use their positive maximum. Floating-point image data uses
    // the stb HDR range [0, 1]. This rule prevents valid HDR values from becoming black.
    constexpr double srcMax = std::is_floating_point_v<SrcT> ? 1.0 : static_cast<double>(std::numeric_limits<SrcT>::max());
    constexpr double destMax = std::is_floating_point_v<DestT> ? 1.0 : static_cast<double>(std::numeric_limits<DestT>::max());

    const usize elementCount = bytes.size() / sizeof(SrcT);
    auto convertedValues = std::make_unique<DestT[]>(elementCount);
    for(usize i = 0; i < elementCount; ++i)
    {
      const SrcT srcValue = ReadElementAs<SrcT>(bytes.data(), i * sizeof(SrcT));
      // Clamp before normalization so exterior values do not wrap in the destination type.
      const double clampedSrc = std::clamp(static_cast<double>(srcValue), 0.0, srcMax);
      const double normalized = clampedSrc / srcMax;
      convertedValues[i] = static_cast<DestT>(normalized * destMax);
    }
    return dataStore.copyFromBuffer(destinationOffset, nonstd::span<const DestT>(convertedValues.get(), elementCount));
  }
};

/**
 * @struct DispatchConversionFunctor
 * @brief Dispatches the destination type after source-type dispatch.
 */
struct DispatchConversionFunctor
{
  /**
   * @brief Invokes one source/destination conversion specialization.
   * @tparam SrcT Specifies the decoder scalar type.
   * @param destType Selects the destination scalar type.
   * @param dataArray Receives converted values.
   * @param bytes Provides a complete number of source values.
   * @param destinationOffset Specifies the first destination value.
   * @return Destination bulk-write result.
   */
  template <typename SrcT>
  Result<> operator()(DataType destType, IDataArray& dataArray, std::span<const uint8> bytes, usize destinationOffset)
  {
    return ExecuteDataFunction(ConvertPixelDataFunctor<SrcT>{}, destType, dataArray, bytes, destinationOffset);
  }
};
} // namespace

ReadImage::ReadImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadImage::~ReadImage() noexcept = default;

Result<> ReadImage::operator()()
{
  const auto& inputFilePath = m_InputValues.inputFilePath;

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Reading image file: {}", inputFilePath.string()));

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
      // Interpret physical bounds with the spatial metadata used during preflight.
      // Preprocessed mode applies requested overrides before this conversion.
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

  const bool convertData = m_InputValues.changeDataType && srcType != destType;
  if(convertData)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Converting pixel data from {} to {}", DataTypeToString(srcType), DataTypeToString(destType)));
  }

  const usize bytesPerComponent = GetDataTypeSize(srcType);
  const usize bytesPerPixel = window.numComponents * bytesPerComponent;
  bool cancelled = false;
  // The decoder streams row segments directly to the destination. Cancellation
  // returns success and retains segments written before the callback stops.
  Result<> streamResult = imageIO->readPixelDataRows(inputFilePath, [&](usize sourceRow, usize sourceColumn, usize pixelCount, std::span<const uint8> pixels) -> Result<> {
    if(m_ShouldCancel)
    {
      cancelled = true;
      return MakeErrorResult(-2004, "Image read cancelled.");
    }

    if(sourceRow < window.yStart || sourceRow >= window.yStart + window.dstHeight)
    {
      return {};
    }

    const usize segmentEnd = sourceColumn + pixelCount;
    const usize cropEnd = window.xStart + window.dstWidth;
    const usize copyStart = std::max(sourceColumn, window.xStart);
    const usize copyEnd = std::min(segmentEnd, cropEnd);
    if(copyStart >= copyEnd)
    {
      return {};
    }

    const usize copiedPixels = copyEnd - copyStart;
    const usize sourceByteOffset = (copyStart - sourceColumn) * bytesPerPixel;
    const usize copiedBytes = copiedPixels * bytesPerPixel;
    const std::span<const uint8> copiedSpan = pixels.subspan(sourceByteOffset, copiedBytes);
    const usize destinationOffset = (((sourceRow - window.yStart) * window.dstWidth) + (copyStart - window.xStart)) * window.numComponents;

    if(convertData)
    {
      return ExecuteDataFunction(DispatchConversionFunctor{}, srcType, destType, imageArray, copiedSpan, destinationOffset);
    }
    return ExecuteDataFunction(CopyPixelDataFunctor{}, srcType, imageArray, copiedSpan, destinationOffset);
  });
  if(cancelled)
  {
    return {};
  }
  return streamResult;
}

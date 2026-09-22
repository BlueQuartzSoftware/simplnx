#include "ReadImage.hpp"

#include "ImageProcessing/Filters/Algorithms/ReadNrrdFile.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <type_traits>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
/**
 * @brief Geometry of one raster read (2D raster or multi-page TIFF Z-stack).
 *
 * Source dimensions describe one decoded page. Destination dimensions describe the post-crop output,
 * and the start coordinates select its source-space corner. Destination page dz reads source page
 * zStart + dz unless singlePageOnly restricts the read to page zero.
 */
struct RasterStreamInfo
{
  usize srcWidth = 0;
  usize srcHeight = 0;
  usize numComponents = 1;
  usize dstWidth = 0;
  usize dstHeight = 0;
  usize dstDepth = 1;
  usize xStart = 0;
  usize yStart = 0;
  usize zStart = 0;
  bool singlePageOnly = false;
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
 * @brief Bulk-writes decoder bytes whose scalar type matches the destination.
 *
 * destinationOffset is a component (element) offset into the destination store.
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
 * @brief Converts one decoder row or band from SrcT to a normalized destination scalar type.
 *
 * Uses the same saturation and normalization convention as the whole-image path.
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

// Reads the NRRD backend by reusing the existing ReadNrrdFile algorithm verbatim -- the NRRD header
// parse, crop-bounds resolution, and per-scanline streaming all live there. ReadImage only maps its
// own inputs onto ReadNrrdFileInputValues so there is a single NRRD implementation.
Result<> ReadNrrdBackend(DataStructure& dataStructure, const ReadImageInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  ReadNrrdFileInputValues nrrdInputs;
  nrrdInputs.InputFilePath = inputValues.inputFilePath;
  nrrdInputs.ImageGeometryPath = inputValues.imageGeometryPath;
  nrrdInputs.CellAttributeMatrixName = inputValues.cellDataName;
  nrrdInputs.ImageDataArrayName = inputValues.imageDataArrayPath.getTargetName();
  nrrdInputs.CroppingOptions = inputValues.croppingOptions;

  return ReadNrrdFile(dataStructure, messageHandler, shouldCancel, &nrrdInputs)();
}

// Reads the raster backend (2D raster or multi-page TIFF Z-stack). Each selected source page is streamed
// row/tile segment at a time directly into the destination store (bounded row streaming = the OOC memory
// contract), so no whole-page staging buffer is ever allocated.
Result<> ReadRasterBackend(DataStructure& dataStructure, const ReadImageInputValues& inputValues, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const auto& inputFilePath = inputValues.inputFilePath;

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

  auto& imageArray = dataStructure.getDataRefAs<IDataArray>(inputValues.imageDataArrayPath);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.imageGeometryPath);
  const SizeVec3 geomDims = imageGeom.getDimensions(); // final (post-crop) destination dims: X, Y, Z

  RasterStreamInfo stream;
  stream.srcWidth = metadata.width;
  stream.srcHeight = metadata.height;
  stream.numComponents = metadata.numComponents;
  stream.dstWidth = geomDims[0];
  stream.dstHeight = geomDims[1];
  // When only page 0 is requested (ReadImageStack sub-call), the geometry is Z==1 and no Z crop
  // applies; otherwise the destination depth is whatever the (possibly Z-cropped) geometry has.
  stream.dstDepth = inputValues.readSinglePageOnly ? 1 : geomDims[2];
  stream.singlePageOnly = inputValues.readSinglePageOnly;

  // Resolve the source-space start offsets (xStart, yStart, zStart). X/Y cropping mirrors the prior
  // 2D behavior exactly; Z cropping is only honored for a genuine multi-page volume that is not being
  // read as a single page. The destination extents (dstWidth/dstHeight/dstDepth) already come from the
  // cropped geometry that preflight produced, so here we only need the start corner.
  const auto& croppingOptions = inputValues.croppingOptions;
  const bool cropImage = croppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping;
  const bool zCropActive = cropImage && croppingOptions.cropZ && !inputValues.readSinglePageOnly && metadata.numPages > 1;
  if(cropImage)
  {
    if(croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
    {
      if(croppingOptions.cropX)
      {
        stream.xStart = static_cast<usize>(croppingOptions.xBoundVoxels[0]);
      }
      if(croppingOptions.cropY)
      {
        stream.yStart = static_cast<usize>(croppingOptions.yBoundVoxels[0]);
      }
      if(zCropActive)
      {
        stream.zStart = static_cast<usize>(croppingOptions.zBoundVoxels[0]);
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
      if(inputValues.originSpacingProcessing == OriginSpacingProcessing::Preprocessed)
      {
        if(inputValues.changeSpacing)
        {
          srcSpacing = inputValues.spacing;
        }
        if(inputValues.changeOrigin)
        {
          srcOrigin = inputValues.origin;
          if(inputValues.centerOrigin)
          {
            srcOrigin[0] = -0.5f * srcSpacing[0] * static_cast<float32>(metadata.width);
            srcOrigin[1] = -0.5f * srcSpacing[1] * static_cast<float32>(metadata.height);
            srcOrigin[2] = -0.5f * srcSpacing[2] * static_cast<float32>(metadata.numPages);
          }
        }
      }
      if(croppingOptions.cropX)
      {
        const float64 xMin = static_cast<float64>(croppingOptions.xBoundPhysical[0]);
        const int64 voxelX = static_cast<int64>((xMin - static_cast<float64>(srcOrigin[0])) / static_cast<float64>(srcSpacing[0]));
        if(voxelX < 0 || static_cast<usize>(voxelX) >= stream.srcWidth)
        {
          return MakeErrorResult(-2002, fmt::format("Physical crop X minimum {} is outside the source image extent [{}, {}) given file origin {} and spacing {}", xMin, srcOrigin[0],
                                                    srcOrigin[0] + srcSpacing[0] * static_cast<float32>(stream.srcWidth), srcOrigin[0], srcSpacing[0]));
        }
        stream.xStart = static_cast<usize>(voxelX);
      }
      if(croppingOptions.cropY)
      {
        const float64 yMin = static_cast<float64>(croppingOptions.yBoundPhysical[0]);
        const int64 voxelY = static_cast<int64>((yMin - static_cast<float64>(srcOrigin[1])) / static_cast<float64>(srcSpacing[1]));
        if(voxelY < 0 || static_cast<usize>(voxelY) >= stream.srcHeight)
        {
          return MakeErrorResult(-2003, fmt::format("Physical crop Y minimum {} is outside the source image extent [{}, {}) given file origin {} and spacing {}", yMin, srcOrigin[1],
                                                    srcOrigin[1] + srcSpacing[1] * static_cast<float32>(stream.srcHeight), srcOrigin[1], srcSpacing[1]));
        }
        stream.yStart = static_cast<usize>(voxelY);
      }
      if(zCropActive)
      {
        const float64 zMin = static_cast<float64>(croppingOptions.zBoundPhysical[0]);
        const int64 voxelZ = static_cast<int64>((zMin - static_cast<float64>(srcOrigin[2])) / static_cast<float64>(srcSpacing[2]));
        if(voxelZ < 0 || static_cast<usize>(voxelZ) >= metadata.numPages)
        {
          return MakeErrorResult(-2004, fmt::format("Physical crop Z minimum {} is outside the source image extent [{}, {}) given file origin {} and spacing {}", zMin, srcOrigin[2],
                                                    srcOrigin[2] + srcSpacing[2] * static_cast<float32>(metadata.numPages), srcOrigin[2], srcSpacing[2]));
        }
        stream.zStart = static_cast<usize>(voxelZ);
      }
    }
  }

  if(stream.xStart + stream.dstWidth > stream.srcWidth || stream.yStart + stream.dstHeight > stream.srcHeight || stream.zStart + stream.dstDepth > metadata.numPages)
  {
    return MakeErrorResult(-2001, fmt::format("Crop window (start=[{},{},{}], size=[{},{},{}]) does not fit within the source image (size=[{},{},{}])", stream.xStart, stream.yStart, stream.zStart,
                                              stream.dstWidth, stream.dstHeight, stream.dstDepth, stream.srcWidth, stream.srcHeight, metadata.numPages));
  }

  const DataType destType = imageArray.getDataType();
  const DataType srcType = metadata.dataType;

  const bool convertData = inputValues.changeDataType && srcType != destType;
  if(convertData)
  {
    messageHandler(IFilter::Message::Type::Info, fmt::format("Converting pixel data from {} to {}", DataTypeToString(srcType), DataTypeToString(destType)));
  }

  const usize bytesPerComponent = GetDataTypeSize(srcType);
  const usize bytesPerPixel = stream.numComponents * bytesPerComponent;
  const usize sliceElements = stream.dstWidth * stream.dstHeight * stream.numComponents; // destination component (element) count per Z slice

  // Stream each selected page directly into the destination store. Every write offset folds in the Z
  // slice base (dz * sliceElements) so a multi-page volume lands each page at its correct depth, and the
  // XY crop is applied per row segment so only the requested window is written.
  bool cancelled = false;
  Result<> readResult;
  for(usize dz = 0; dz < stream.dstDepth; dz++)
  {
    if(shouldCancel)
    {
      cancelled = true;
      break;
    }
    const usize sourcePage = stream.singlePageOnly ? 0 : (stream.zStart + dz);
    const usize destSliceBase = dz * sliceElements;

    // Per-page geometry check: the destination offsets are computed from page-0-derived dimensions, so a
    // page whose dimensions / component count / scalar type differ would silently mis-map. Because the
    // IImageIO interface exposes metadata only for page 0, we validate each page against the observed
    // extent of its own decoded segments and reject any mismatch rather than mis-sizing the slice.
    usize observedRows = 0;
    usize observedColumns = 0;
    bool pixelStrideMismatch = false;

    readResult = imageIO->readPixelDataRows(
        inputFilePath,
        [&](usize sourceRow, usize sourceColumn, usize pixelCount, std::span<const uint8> pixels) -> Result<> {
          if(shouldCancel)
          {
            cancelled = true;
            return MakeErrorResult(-2006, "Image read cancelled.");
          }

          // Track the page's true extent (before cropping) so it can be validated against page 0.
          observedRows = std::max(observedRows, sourceRow + 1);
          observedColumns = std::max(observedColumns, sourceColumn + pixelCount);
          if(pixels.size() != pixelCount * bytesPerPixel)
          {
            pixelStrideMismatch = true;
          }

          // Y crop: skip rows outside the destination window.
          if(sourceRow < stream.yStart || sourceRow >= stream.yStart + stream.dstHeight)
          {
            return {};
          }

          // X crop: intersect this segment with the destination column window.
          const usize segmentEnd = sourceColumn + pixelCount;
          const usize cropEnd = stream.xStart + stream.dstWidth;
          const usize copyStart = std::max(sourceColumn, stream.xStart);
          const usize copyEnd = std::min(segmentEnd, cropEnd);
          if(copyStart >= copyEnd)
          {
            return {};
          }

          const usize copiedPixels = copyEnd - copyStart;
          const usize sourceByteOffset = (copyStart - sourceColumn) * bytesPerPixel;
          const usize copiedBytes = copiedPixels * bytesPerPixel;
          const std::span<const uint8> copiedSpan = pixels.subspan(sourceByteOffset, copiedBytes);
          const usize destinationOffset = destSliceBase + (((sourceRow - stream.yStart) * stream.dstWidth) + (copyStart - stream.xStart)) * stream.numComponents;

          if(convertData)
          {
            return ExecuteDataFunction(DispatchConversionFunctor{}, srcType, destType, imageArray, copiedSpan, destinationOffset);
          }
          return ExecuteDataFunction(CopyPixelDataFunctor{}, srcType, imageArray, copiedSpan, destinationOffset);
        },
        sourcePage);

    if(cancelled)
    {
      return {};
    }
    if(readResult.invalid())
    {
      return readResult;
    }

    if(observedRows != stream.srcHeight || observedColumns != stream.srcWidth || pixelStrideMismatch)
    {
      return MakeErrorResult(-2007, fmt::format("Page {} of '{}' has dimensions {}x{} (or a differing component count / scalar type) that do not match page 0 ({}x{}). Every page of a "
                                                "multi-page volume must share identical dimensions, component count, and scalar type.",
                                                sourcePage, inputFilePath.string(), observedColumns, observedRows, stream.srcWidth, stream.srcHeight));
    }
  }

  // Single-page-only reads are driven by ReadImageStack, which contributes one slice per file. If such
  // a file is a multi-page TIFF, every page past the first is intentionally discarded, so warn to make
  // the dropped data explicit. The warning is attached to the Result (so it propagates into the stack's
  // execute result) and also emitted through the message handler. It never fires for a normal standalone
  // read, where readSinglePageOnly is false and all pages are read into a genuine 3D volume.
  if(readResult.valid() && inputValues.readSinglePageOnly && metadata.numPages > 1)
  {
    std::string warningMessage = fmt::format("Image file '{}' has {} pages; reading only the first page (page 0).", inputFilePath.string(), metadata.numPages);
    messageHandler(IFilter::Message::Type::Warning, warningMessage);
    readResult.warnings().push_back(Warning{-2005, std::move(warningMessage)});
  }

  return readResult;
}
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

  if(DetermineReadImageBackend(inputFilePath) == ReadImageBackend::Nrrd)
  {
    return ReadNrrdBackend(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel);
  }

  return ReadRasterBackend(m_DataStructure, m_InputValues, m_MessageHandler, m_ShouldCancel);
}

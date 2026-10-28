#include "WriteImageDirect.hpp"

#include "WriteImageColorUtilities.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/IImageIO.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"
#include "simplnx/Utilities/ScaleBarRenderer.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// Applies the selected flip to one packed, row-major slice buffer.
void ApplyImageFlip(std::vector<uint8>& buffer, usize width, usize height, usize pixelStrideBytes, ImageFlipTransform flip)
{
  if(flip == ImageFlipTransform::None || width == 0 || height == 0)
  {
    return;
  }
  const usize rowBytes = width * pixelStrideBytes;
  if(flip == ImageFlipTransform::FlipAboutXAxis)
  {
    // Swap top and bottom rows.
    for(usize y = 0; y < height / 2; ++y)
    {
      uint8* rowTop = buffer.data() + y * rowBytes;
      uint8* rowBot = buffer.data() + (height - 1 - y) * rowBytes;
      std::swap_ranges(rowTop, rowTop + rowBytes, rowBot);
    }
  }
  else if(flip == ImageFlipTransform::FlipAboutYAxis)
  {
    // Swap left and right pixels within each row.
    for(usize y = 0; y < height; ++y)
    {
      uint8* row = buffer.data() + y * rowBytes;
      for(usize x = 0; x < width / 2; ++x)
      {
        uint8* pixL = row + x * pixelStrideBytes;
        uint8* pixR = row + (width - 1 - x) * pixelStrideBytes;
        std::swap_ranges(pixL, pixL + pixelStrideBytes, pixR);
      }
    }
  }
}

// Converts one uint8 grayscale, RGB, or RGBA slice to RGB for scale-bar output.
// The conversion replicates grayscale values and removes the RGBA alpha value.
std::vector<uint8> ConvertUInt8ToRgb(const std::vector<uint8>& buffer, usize pixelCount, usize numComps)
{
  std::vector<uint8> rgb(pixelCount * 3);
  for(usize i = 0; i < pixelCount; i++)
  {
    if(numComps == 1)
    {
      rgb[i * 3 + 0] = buffer[i];
      rgb[i * 3 + 1] = buffer[i];
      rgb[i * 3 + 2] = buffer[i];
    }
    else
    {
      rgb[i * 3 + 0] = buffer[i * numComps + 0];
      rgb[i * 3 + 1] = buffer[i * numComps + 1];
      rgb[i * 3 + 2] = buffer[i * numComps + 2];
    }
  }
  return rgb;
}

// Converts a plane and slice index to the exact three-dimensional DataStore extent.
Extent CreateSliceExtent(usize planeIndex, usize sliceIndex, usize dimX, usize dimY, usize dimZ)
{
  if(planeIndex == 0)
  {
    return {{sliceIndex, 0, 0}, {sliceIndex, dimY - 1, dimX - 1}};
  }
  if(planeIndex == 1)
  {
    return {{0, sliceIndex, 0}, {dimZ - 1, sliceIndex, dimX - 1}};
  }
  return {{0, 0, sliceIndex}, {dimZ - 1, dimY - 1, sliceIndex}};
}

/**
 * @struct ExtractSliceFunctor
 * @brief Extracts one typed DataStore slice into packed bytes.
 */
struct ExtractSliceFunctor
{
  // The operation reads one typed extent and expands Boolean values into bytes.
  template <typename T>
  Result<> operator()(const IDataArray& dataArray, std::vector<uint8>& buffer, usize sliceIndex, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize nComp,
                      const std::function<void(usize, usize)>& reportRows = {})
  {
    const auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const Extent sliceExtent = CreateSliceExtent(planeIndex, sliceIndex, dimX, dimY, dimZ);
    Result<std::vector<T>> sliceResult = dataStore.readExtent(sliceExtent);
    if(sliceResult.invalid())
    {
      return ConvertResult(std::move(sliceResult));
    }
    const std::vector<T>& sliceValues = sliceResult.value();
    const usize expectedElements = static_cast<usize>(sliceExtent.totalElements()) * nComp;
    if(sliceValues.size() != expectedElements)
    {
      return MakeErrorResult(-27023, fmt::format("Could not bulk-read image slice {}. Expected {} values but received {}.", sliceIndex, expectedElements, sliceValues.size()));
    }

    const usize rowElements = (planeIndex == 2 ? dimY : dimX) * nComp;
    const usize rowCount = planeIndex == 0 ? dimY : dimZ;
    for(usize row = 0; row < rowCount; ++row)
    {
      const usize offset = row * rowElements;
      if constexpr(std::is_same_v<T, bool>)
      {
        for(usize i = 0; i < rowElements; ++i)
        {
          buffer[offset + i] = sliceValues[offset + i] ? 1 : 0;
        }
      }
      else
      {
        std::memcpy(buffer.data() + offset * sizeof(T), sliceValues.data() + offset, rowElements * sizeof(T));
      }
      if(reportRows)
      {
        reportRows(row + 1, rowCount);
      }
    }

    return {};
  }
};

// Reads one optional Boolean or uint8 mask extent into a byte vector.
Result<std::vector<uint8>> ReadMaskSlice(const IDataArray* maskArray, const Extent& extent)
{
  if(maskArray == nullptr)
  {
    return {std::vector<uint8>{}};
  }
  if(maskArray->getDataType() == DataType::boolean)
  {
    const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
    Result<std::vector<bool>> maskResult = maskStore.readExtent(extent);
    if(maskResult.invalid())
    {
      return ConvertInvalidResult<std::vector<uint8>>(std::move(maskResult));
    }
    const std::vector<bool>& boolMask = maskResult.value();
    std::vector<uint8> mask(boolMask.size());
    std::transform(boolMask.cbegin(), boolMask.cend(), mask.begin(), [](bool value) { return value ? uint8{1} : uint8{0}; });
    return ConvertResultTo<std::vector<uint8>>(ConvertResult(std::move(maskResult)), std::move(mask));
  }
  const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
  return maskStore.readExtent(extent);
}

/**
 * @struct ColorizeVolumeFunctor
 * @brief Colorizes and writes a scalar volume one slice at a time.
 *
 * Global min/max is reduced through fixed pages; each scalar and mask slice is
 * then gathered, colorized, and released before the next slice. This avoids
 * staging a full RGB volume for OOC inputs.
 */
struct ColorizeVolumeFunctor
{
  // The operation calculates the range in pages and then colorizes bounded slices.
  template <typename T>
  Result<> operator()(const IDataArray& dataArrayRef, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize sliceCount, usize sliceW, usize sliceH, const std::vector<float32>& binPoints,
                      const std::vector<float32>& controlPoints, const IDataArray* maskArray, const std::vector<uint8>& invalidColor, uint64 nonfinitePolicy, const std::atomic_bool& shouldCancel,
                      const std::function<Result<>(std::vector<uint8>&, usize)>& writeSlice, const std::function<void(usize, usize)>& reportRange = {},
                      const std::function<void(usize, usize)>& reportRows = {})
  {
    if(Result<> sizeResult = WriteImageColor::ValidateSize<T>(sliceW, sliceH, sliceCount, maskArray != nullptr); sizeResult.invalid())
    {
      return sizeResult;
    }
    auto rangeResult = WriteImageColor::ScanRange<T>(dataArrayRef, maskArray, nonfinitePolicy, shouldCancel, reportRange);
    if(rangeResult.invalid())
    {
      return ConvertResult(std::move(rangeResult));
    }
    const auto range = rangeResult.value();
    Result<> status = ConvertResult(std::move(rangeResult));
    if(range.Cancelled)
    {
      return status;
    }
    const auto& dataStore = dataArrayRef.template getIDataStoreRefAs<AbstractDataStore<T>>();
    std::vector<uint8> sliceBuffer(sliceW * sliceH * 3);

    for(usize slice = 0; slice < sliceCount; ++slice)
    {
      if(shouldCancel)
      {
        return status;
      }

      const Extent sliceExtent = CreateSliceExtent(planeIndex, slice, dimX, dimY, dimZ);
      Result<std::vector<T>> sliceResult = dataStore.readExtent(sliceExtent);
      if(sliceResult.invalid())
      {
        return MergeResults(std::move(status), ConvertResult(std::move(sliceResult)));
      }
      status.warnings().insert(status.warnings().end(), sliceResult.warnings().begin(), sliceResult.warnings().end());
      const std::vector<T>& sliceValues = sliceResult.value();
      const usize pixelCount = sliceW * sliceH;
      if(sliceValues.size() != pixelCount)
      {
        return MergeResults(std::move(status),
                            MakeErrorResult(-27024, fmt::format("Could not bulk-read colorized image slice {}. Expected {} values but received {}.", slice, pixelCount, sliceValues.size())));
      }
      Result<std::vector<uint8>> maskResult = ReadMaskSlice(maskArray, sliceExtent);
      if(maskResult.invalid())
      {
        return MergeResults(std::move(status), ConvertResult(std::move(maskResult)));
      }
      status.warnings().insert(status.warnings().end(), maskResult.warnings().begin(), maskResult.warnings().end());
      const std::vector<uint8>& maskValues = maskResult.value();
      if(maskArray != nullptr && maskValues.size() != pixelCount)
      {
        return MergeResults(std::move(status), MakeErrorResult(-27025, fmt::format("Could not bulk-read mask slice {}. Expected {} values but received {}.", slice, pixelCount, maskValues.size())));
      }

      for(usize row = 0; row < sliceH; ++row)
      {
        if(shouldCancel)
        {
          return status;
        }
        for(usize col = 0; col < sliceW; ++col)
        {
          const usize dstPixel = row * sliceW + col;
          const usize dst = dstPixel * 3;
          if(maskArray != nullptr && maskValues[dstPixel] == 0)
          {
            sliceBuffer[dst + 0] = invalidColor[0];
            sliceBuffer[dst + 1] = invalidColor[1];
            sliceBuffer[dst + 2] = invalidColor[2];
            continue;
          }

          const T sourceValue = sliceValues[dstPixel];
          const float32 nValue = WriteImageColor::Normalize(sourceValue, range.Minimum, range.Maximum);
          const std::array<uint8, 3> rgb = WriteImageColor::ComputeRgb(nValue, binPoints, controlPoints);
          sliceBuffer[dst + 0] = rgb[0];
          sliceBuffer[dst + 1] = rgb[1];
          sliceBuffer[dst + 2] = rgb[2];
        }
        if(reportRows)
        {
          reportRows(row + 1, sliceH);
        }
      }

      status = MergeResults(std::move(status), writeSlice(sliceBuffer, slice));
      if(status.invalid())
      {
        return status;
      }
    }
    return status;
  }
};
} // namespace

// -----------------------------------------------------------------------------
WriteImageDirect::WriteImageDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

// -----------------------------------------------------------------------------
WriteImageDirect::~WriteImageDirect() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteImageDirect::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues.imageGeometryPath);
  const auto& imageArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues.imageDataArrayPath);

  // ImageGeom stores dimensions in X, Y, Z order.
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

  const bool addScaleBar = m_InputValues.addScaleBar;
  usize bandHeight = 0;
  std::vector<uint8> bandRgb;
  if(addScaleBar)
  {
    FloatVec3 spacing = imageGeom.getSpacing();
    const float64 unitsPerPixel = static_cast<float64>((m_InputValues.planeIndex == 2) ? spacing[1] : spacing[0]);
    bandHeight = ScaleBarRenderer::ComputeBandHeight(sliceH);
    bandRgb = ScaleBarRenderer::RenderScaleBarBandRgb(sliceW, sliceH, unitsPerPixel, imageGeom.getUnits());
  }

  // The component count and byte size are constant across slices.
  // Calculate them once outside writeSlice.
  const usize incomingComps = m_InputValues.createColorTable ? 3 : nComp;
  const usize incomingTypeSize = m_InputValues.createColorTable ? GetDataTypeSize(DataType::uint8) : bytesPerComponent;

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

  // All slices share metadata. Color and scale-bar modes write three UInt8 components.
  ImageMetadata metadata;
  metadata.width = sliceW;
  metadata.height = sliceH + bandHeight;
  metadata.numComponents = (m_InputValues.createColorTable || addScaleBar) ? 3 : nComp;
  metadata.dataType = (m_InputValues.createColorTable || addScaleBar) ? DataType::uint8 : dataType;
  metadata.numPages = 1;
  const FloatVec3 origin = imageGeom.getOrigin();
  const FloatVec3 spacing = imageGeom.getSpacing();
  if(m_InputValues.planeIndex == 0)
  {
    metadata.origin = FloatVec3{origin[0], origin[1], 0.0f};
    metadata.spacing = FloatVec3{spacing[0], spacing[1], 1.0f};
  }
  else if(m_InputValues.planeIndex == 1)
  {
    metadata.origin = FloatVec3{origin[0], origin[2], 0.0f};
    metadata.spacing = FloatVec3{spacing[0], spacing[2], 1.0f};
  }
  else
  {
    metadata.origin = FloatVec3{origin[1], origin[2], 0.0f};
    metadata.spacing = FloatVec3{spacing[1], spacing[2], 1.0f};
  }

  // All dispatch and image callbacks below run synchronously on this thread.
  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  progressThrottle.reset(sliceCount, "Writing image slices");
  m_MessageHandler.sendInfoMessage("Preparing and writing image slices");
  const std::function<void(usize, usize)> reportRange = [&](usize current, usize total) { progressThrottle.updatePercent("Computing color range", current, total); };
  const std::function<void(usize, usize)> reportRows = [&](usize current, usize total) {
    if(sliceCount == 1)
    {
      progressThrottle.updatePercent("Preparing image rows", current, total);
    }
  };

  // Encode and atomically commit one independently named slice file.
  auto writeSlice = [&](std::vector<uint8>& sliceBuffer, usize slice) -> Result<> {
    // Add an index only when the selection produces multiple slices.
    fs::path slicePath = parent / fmt::format("{}{}", stem.string(), ext.string());
    if(sliceCount > 1)
    {
      std::string indexStr = CreateIndexString(slice + m_InputValues.indexOffset, static_cast<usize>(m_InputValues.totalIndexDigits), m_InputValues.leadingDigitCharacter);
      slicePath = parent / fmt::format("{}_{}{}", stem.string(), indexStr, ext.string());
    }

    auto atomicFileResult = AtomicFile::Create(slicePath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    // Flip before padding so the scale bar stays upright at the output bottom.
    ApplyImageFlip(sliceBuffer, sliceW, sliceH, incomingComps * incomingTypeSize, m_InputValues.flipMode);

    std::vector<uint8>* writeBufferPtr = &sliceBuffer;
    std::vector<uint8> paddedBuffer;
    if(addScaleBar)
    {
      // Preflight restricts scale-bar input to UInt8 or color-table RGB.
      paddedBuffer = ConvertUInt8ToRgb(sliceBuffer, sliceW * sliceH, incomingComps);
      paddedBuffer.insert(paddedBuffer.end(), bandRgb.begin(), bandRgb.end());
      writeBufferPtr = &paddedBuffer;
    }

    if(sliceCount == 1)
    {
      m_MessageHandler.sendInfoMessage("Encoding image file");
    }
    auto writeResult = imageIO->writePixelData(atomicFile.tempFilePath(), *writeBufferPtr, metadata);
    if(m_InputValues.createColorTable)
    {
      Result<> fileStatus;
      fileStatus.warnings() = std::move(atomicFileResult.warnings());
      writeResult = MergeResults(std::move(fileStatus), std::move(writeResult));
    }
    if(writeResult.invalid())
    {
      return writeResult;
    }
    if(m_ShouldCancel)
    {
      if(m_InputValues.createColorTable)
      {
        return MergeResults(std::move(writeResult), MakeErrorResult(-1, "Filter cancelled"));
      }
      return MakeErrorResult(-1, "Filter cancelled");
    }
    auto commitResult = atomicFile.commit();
    if(m_InputValues.createColorTable)
    {
      commitResult = MergeResults(std::move(writeResult), std::move(commitResult));
    }
    if(commitResult.valid())
    {
      progressThrottle.updateCount(slice + 1);
      if(slice + 1 == sliceCount)
      {
        m_MessageHandler.sendProgressCount("Writing image slices", sliceCount, sliceCount);
      }
    }
    return commitResult;
  };

  if(m_InputValues.createColorTable)
  {
    auto controlPointsResult = ColorTableUtilities::ExtractControlPoints(m_InputValues.presetName);
    if(controlPointsResult.invalid())
    {
      return ConvertResult(std::move(controlPointsResult));
    }
    std::vector<float32> controlPoints = controlPointsResult.value();
    if(controlPoints.empty())
    {
      return MakeErrorResult(-27021, fmt::format("No valid control points found for preset '{}'", m_InputValues.presetName));
    }
    // Each control color has four values in [A,R,G,B] order.
    // Interpolation requires two colors to prevent an out-of-bounds read.
    if(controlPoints.size() < 8)
    {
      return MakeErrorResult(-27022, fmt::format("Preset '{}' must define at least 2 control colors", m_InputValues.presetName));
    }
    std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);
    if(Result<> paletteResult = WriteImageColor::ValidatePalette(controlPoints, binPoints); paletteResult.invalid())
    {
      return MergeResults(ConvertResult(std::move(controlPointsResult)), std::move(paletteResult));
    }

    const IDataArray* maskArrayPtr = nullptr;
    if(m_InputValues.useMask)
    {
      maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues.maskArrayPath);
    }

    auto colorResult =
        ExecuteDataFunction(ColorizeVolumeFunctor{}, dataType, imageArray, m_InputValues.planeIndex, dimX, dimY, dimZ, sliceCount, sliceW, sliceH, binPoints, controlPoints, maskArrayPtr,
                            m_InputValues.invalidColor, m_InputValues.NonfiniteInputHandling, m_ShouldCancel, std::function<Result<>(std::vector<uint8>&, usize)>(writeSlice), reportRange, reportRows);
    return MergeResults(ConvertResult(std::move(controlPointsResult)), std::move(colorResult));
  }

  // The non-color path preserves typed bytes for one slice at a time.
  usize sliceBufferSize = sliceW * sliceH * nComp * bytesPerComponent;
  std::vector<uint8> sliceBuffer(sliceBufferSize);
  for(usize slice = 0; slice < sliceCount; ++slice)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto extractResult = ExecuteDataFunction(ExtractSliceFunctor{}, dataType, imageArray, sliceBuffer, slice, m_InputValues.planeIndex, dimX, dimY, dimZ, nComp, reportRows);
    if(extractResult.invalid())
    {
      return extractResult;
    }
    auto sliceResult = writeSlice(sliceBuffer, slice);
    if(sliceResult.invalid())
    {
      return sliceResult;
    }
  }
  return {};
}

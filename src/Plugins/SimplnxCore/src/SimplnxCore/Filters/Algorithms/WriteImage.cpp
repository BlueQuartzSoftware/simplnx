#include "WriteImage.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Extent.hpp"
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

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
/**
 * @brief Flips one packed row-major slice in place.
 * @param buffer Provides and receives pixel bytes.
 * @param width Specifies pixels per row.
 * @param height Specifies row count.
 * @param pixelStrideBytes Specifies bytes per pixel.
 * @param flip Selects horizontal, vertical, or no flip.
 * @pre buffer contains at least width times height times pixelStrideBytes bytes.
 */
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

/**
 * @brief Converts one bounded UInt8 grayscale/RGB/RGBA slice to RGB for scale-bar rendering.
 * @param buffer Provides packed UInt8 pixels.
 * @param pixelCount Specifies source pixels.
 * @param numComps Specifies one, three, or four source components.
 * @return Packed RGB pixels.
 * @pre buffer contains pixelCount times numComps values.
 *
 * Grayscale is replicated and RGBA alpha is dropped. Preflight guarantees the
 * UInt8 component format whenever this conversion is required.
 */
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

/**
 * @brief Creates the three-dimensional extent for one image slice.
 * @param planeIndex Selects XY, XZ, or YZ.
 * @param sliceIndex Specifies the fixed-axis index.
 * @param dimX Specifies X cells.
 * @param dimY Specifies Y cells.
 * @param dimZ Specifies Z cells.
 * @return Inclusive source extent.
 * @pre Dimensions are positive and sliceIndex is valid for planeIndex.
 */
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
  /**
   * @brief Reads and packs one typed slice.
   * @tparam T Specifies the source scalar type.
   * @param dataArray Provides source values.
   * @param buffer Receives packed bytes.
   * @param sliceIndex Specifies the fixed-axis index.
   * @param planeIndex Selects XY, XZ, or YZ.
   * @param dimX Specifies X cells.
   * @param dimY Specifies Y cells.
   * @param dimZ Specifies Z cells.
   * @param nComp Specifies components per cell.
   * @return Size mismatch error, or success.
   *
   * Boolean values expand to one byte. Other types preserve native byte order.
   */
  template <typename T>
  Result<> operator()(const IDataArray& dataArray, std::vector<uint8>& buffer, usize sliceIndex, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize nComp)
  {
    const auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const Extent sliceExtent = CreateSliceExtent(planeIndex, sliceIndex, dimX, dimY, dimZ);
    const std::vector<T> sliceValues = dataStore.readExtent(sliceExtent);
    const usize expectedElements = static_cast<usize>(sliceExtent.totalElements()) * nComp;
    if(sliceValues.size() != expectedElements)
    {
      return MakeErrorResult(-27023, fmt::format("Could not bulk-read image slice {}. Expected {} values but received {}.", sliceIndex, expectedElements, sliceValues.size()));
    }

    if constexpr(std::is_same_v<T, bool>)
    {
      for(usize i = 0; i < expectedElements; ++i)
      {
        buffer[i] = sliceValues[i] ? 1 : 0;
      }
    }
    else
    {
      std::memcpy(buffer.data(), sliceValues.data(), expectedElements * sizeof(T));
    }

    return {};
  }
};

/**
 * @brief Reads one optional mask extent as bytes.
 * @param maskArray Provides Boolean or UInt8 mask values, or null.
 * @param extent Specifies the source slice.
 * @return Empty vector for no mask, or byte mask values.
 * @pre A non-null maskArray has Boolean or UInt8 type.
 */
Result<std::vector<uint8>> ReadMaskSlice(const IDataArray* maskArray, const Extent& extent)
{
  if(maskArray == nullptr)
  {
    return {std::vector<uint8>{}};
  }
  if(maskArray->getDataType() == DataType::boolean)
  {
    const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
    const std::vector<bool> boolMask = maskStore.readExtent(extent);
    std::vector<uint8> mask(boolMask.size());
    std::transform(boolMask.cbegin(), boolMask.cend(), mask.begin(), [](bool value) { return value ? uint8{1} : uint8{0}; });
    return {std::move(mask)};
  }
  const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
  return {maskStore.readExtent(extent)};
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
  /**
   * @brief Reduces the global range and colorizes typed slices.
   * @tparam T Specifies the source scalar type.
   * @param dataArrayRef Provides scalar cell values.
   * @param planeIndex Selects XY, XZ, or YZ.
   * @param dimX Specifies X cells.
   * @param dimY Specifies Y cells.
   * @param dimZ Specifies Z cells.
   * @param sliceCount Specifies output slices.
   * @param sliceW Specifies output width.
   * @param sliceH Specifies output height before scale-bar padding.
   * @param binPoints Provides normalized color-bin positions.
   * @param controlPoints Provides ARGB control colors.
   * @param numControlColors Specifies control colors.
   * @param maskArray Optionally selects valid pixels.
   * @param invalidColor Provides RGB bytes for invalid pixels.
   * @param shouldCancel Stops before later colorized slices when true.
   * @param writeSlice Encodes and commits one RGB slice.
   * @return Source, mask, colorization, or slice-write error, or success after cancellation.
   *
   * Global range reduction includes masked values and does not check cancellation.
   */
  template <typename T>
  Result<> operator()(const IDataArray& dataArrayRef, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize sliceCount, usize sliceW, usize sliceH, const std::vector<float32>& binPoints,
                      const std::vector<float32>& controlPoints, usize numControlColors, const IDataArray* maskArray, const std::vector<uint8>& invalidColor, const std::atomic_bool& shouldCancel,
                      const std::function<Result<>(std::vector<uint8>&, usize)>& writeSlice)
  {
    const auto& dataStore = dataArrayRef.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numTuples = dataStore.getNumberOfTuples();
    if(numTuples == 0)
    {
      return MakeErrorResult(-27020, "Input array is empty.");
    }

    // Include masked values to match Create Color Map range semantics.
    constexpr usize k_TargetPageBytes = 1024 * 1024;
    const usize pageElements = std::max<usize>(1, std::min(numTuples, k_TargetPageBytes / sizeof(T)));
    auto valuePage = std::make_unique<T[]>(pageElements);
    T arrayMin{};
    T arrayMax{};
    bool initialized = false;
    for(usize offset = 0; offset < numTuples; offset += pageElements)
    {
      const usize count = std::min(pageElements, numTuples - offset);
      if(Result<> result = dataStore.copyIntoBuffer(offset, nonstd::span<T>(valuePage.get(), count)); result.invalid())
      {
        return result;
      }
      for(usize i = 0; i < count; ++i)
      {
        if(!initialized)
        {
          arrayMin = valuePage[i];
          arrayMax = valuePage[i];
          initialized = true;
        }
        else
        {
          arrayMin = std::min(arrayMin, valuePage[i]);
          arrayMax = std::max(arrayMax, valuePage[i]);
        }
      }
    }

    std::vector<uint8> sliceBuffer(sliceW * sliceH * 3);

    for(usize slice = 0; slice < sliceCount; ++slice)
    {
      if(shouldCancel)
      {
        return {};
      }

      const Extent sliceExtent = CreateSliceExtent(planeIndex, slice, dimX, dimY, dimZ);
      const std::vector<T> sliceValues = dataStore.readExtent(sliceExtent);
      const usize pixelCount = sliceW * sliceH;
      if(sliceValues.size() != pixelCount)
      {
        return MakeErrorResult(-27024, fmt::format("Could not bulk-read colorized image slice {}. Expected {} values but received {}.", slice, pixelCount, sliceValues.size()));
      }
      Result<std::vector<uint8>> maskResult = ReadMaskSlice(maskArray, sliceExtent);
      if(maskResult.invalid())
      {
        return ConvertResult(std::move(maskResult));
      }
      const std::vector<uint8>& maskValues = maskResult.value();
      if(maskArray != nullptr && maskValues.size() != pixelCount)
      {
        return MakeErrorResult(-27025, fmt::format("Could not bulk-read mask slice {}. Expected {} values but received {}.", slice, pixelCount, maskValues.size()));
      }

      for(usize row = 0; row < sliceH; ++row)
      {
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
          const float32 nValue = ColorTableUtilities::NormalizeValue(sourceValue, arrayMin, arrayMax);
          const std::array<uint8, 3> rgb = ColorTableUtilities::ComputeRgbFromControlPoints(nValue, binPoints, controlPoints, numControlColors);
          sliceBuffer[dst + 0] = rgb[0];
          sliceBuffer[dst + 1] = rgb[1];
          sliceBuffer[dst + 2] = rgb[2];
        }
      }

      auto result = writeSlice(sliceBuffer, slice);
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }
};
} // namespace

WriteImage::WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteImage::~WriteImage() noexcept = default;

Result<> WriteImage::operator()()
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

  // Component count and byte size stay constant for all output slices.
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

  // Encode and atomically commit one independently named slice file.
  auto writeSlice = [&](std::vector<uint8>& sliceBuffer, usize slice) -> Result<> {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Writing slice {}/{}", slice + 1, sliceCount));
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

    auto writeResult = imageIO->writePixelData(atomicFile.tempFilePath(), *writeBufferPtr, metadata);
    if(writeResult.invalid())
    {
      return writeResult;
    }
    return atomicFile.commit();
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
    // Interpolation needs at least two four-value ARGB control colors.
    if(controlPoints.size() < 8)
    {
      return MakeErrorResult(-27022, fmt::format("Preset '{}' must define at least 2 control colors", m_InputValues.presetName));
    }
    std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);
    const usize numControlColors = controlPoints.size() / 4;

    const IDataArray* maskArrayPtr = nullptr;
    if(m_InputValues.useMask)
    {
      maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues.maskArrayPath);
    }

    return ExecuteDataFunction(ColorizeVolumeFunctor{}, dataType, imageArray, m_InputValues.planeIndex, dimX, dimY, dimZ, sliceCount, sliceW, sliceH, binPoints, controlPoints, numControlColors,
                               maskArrayPtr, m_InputValues.invalidColor, m_ShouldCancel, std::function<Result<>(std::vector<uint8>&, usize)>(writeSlice));
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
    auto extractResult = ExecuteDataFunction(ExtractSliceFunctor{}, dataType, imageArray, sliceBuffer, slice, m_InputValues.planeIndex, dimX, dimY, dimZ, nComp);
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

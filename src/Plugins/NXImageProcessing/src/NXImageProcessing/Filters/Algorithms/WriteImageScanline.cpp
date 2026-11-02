#include "WriteImageScanline.hpp"

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
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/ScaleBarRenderer.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

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
constexpr uint64 k_PreferredSliceGroupBytes = 64ULL * 1024ULL * 1024ULL;

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
    // Reverse row order (mirror top-to-bottom).
    for(usize y = 0; y < height / 2; ++y)
    {
      uint8* rowTop = buffer.data() + y * rowBytes;
      uint8* rowBot = buffer.data() + (height - 1 - y) * rowBytes;
      std::swap_ranges(rowTop, rowTop + rowBytes, rowBot);
    }
  }
  else if(flip == ImageFlipTransform::FlipAboutYAxis)
  {
    // Reverse pixel order within each row (mirror left-to-right).
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
 * @brief Creates one extent that contains adjacent output slices.
 * @param planeIndex Selects the XY, XZ, or YZ plane.
 * @param firstSlice Zero-based index of the first output slice.
 * @param sliceCount Number of adjacent output slices.
 * @param dimX Number of source tuples on the X axis.
 * @param dimY Number of source tuples on the Y axis.
 * @param dimZ Number of source tuples on the Z axis.
 * @return Extent in Z, Y, X tuple order.
 */
Extent CreateSliceGroupExtent(usize planeIndex, usize firstSlice, usize sliceCount, usize dimX, usize dimY, usize dimZ)
{
  const usize lastSlice = firstSlice + sliceCount - 1;
  if(planeIndex == 0)
  {
    return {{firstSlice, 0, 0}, {lastSlice, dimY - 1, dimX - 1}};
  }
  if(planeIndex == 1)
  {
    return {{0, firstSlice, 0}, {dimZ - 1, lastSlice, dimX - 1}};
  }
  return {{0, 0, firstSlice}, {dimZ - 1, dimY - 1, lastSlice}};
}

/**
 * @brief Calculates the number of adjacent slices in one source group.
 * @param planeIndex Selects the XY, XZ, or YZ plane.
 * @param sliceCount Number of output slices.
 * @param sliceBytes Number of source bytes for one output slice.
 * @param grantedBytes Number of bytes in the active working-memory reservation.
 * @return Number of slices in one group.
 *
 * One XY slice matches one default OOC chunk. A larger XY group adds chunk scatter without reducing the required chunk reads.
 */
usize CalculateSlicesPerGroup(usize planeIndex, usize sliceCount, uint64 sliceBytes, uint64 grantedBytes)
{
  if(sliceBytes == 0 || grantedBytes < sliceBytes)
  {
    return 0;
  }
  if(planeIndex == 0)
  {
    return 1;
  }
  return std::max<usize>(1, std::min<usize>(sliceCount, grantedBytes / sliceBytes));
}

/**
 * @brief Returns the preferred reservation for one source group.
 * @param planeIndex Selects the XY, XZ, or YZ plane.
 * @param sliceBytes Number of source bytes for one output slice.
 * @return Preferred working-memory reservation in bytes.
 */
uint64 PreferredSliceGroupBytes(usize planeIndex, uint64 sliceBytes)
{
  return planeIndex == 0 ? sliceBytes : std::max(k_PreferredSliceGroupBytes, sliceBytes);
}

/**
 * @brief Copies one slice from a grouped source extent into a packed byte buffer.
 * @tparam T Specifies the source value type.
 * @param groupValues Contains the grouped extent in Z, Y, X tuple order.
 * @param sliceBuffer Receives one packed output slice.
 * @param localSliceIndex Selects one slice in the group.
 * @param groupSliceCount Number of slices in the group.
 * @param planeIndex Selects the XY, XZ, or YZ plane.
 * @param dimX Number of source tuples on the X axis.
 * @param dimY Number of source tuples on the Y axis.
 * @param dimZ Number of source tuples on the Z axis.
 * @param numComponents Number of interleaved components in each tuple.
 * @param reportRows Optional synchronous callback for completed output rows.
 */
template <typename T>
void CopySliceFromGroup(nonstd::span<const T> groupValues, std::vector<uint8>& sliceBuffer, usize localSliceIndex, usize groupSliceCount, usize planeIndex, usize dimX, usize dimY, usize dimZ,
                        usize numComponents, const std::function<void(usize, usize)>& reportRows = {})
{
  const auto copyValues = [&sliceBuffer, &groupValues](usize destinationOffset, usize sourceOffset, usize valueCount) {
    if constexpr(std::is_same_v<T, bool>)
    {
      for(usize valueIndex = 0; valueIndex < valueCount; ++valueIndex)
      {
        sliceBuffer[destinationOffset + valueIndex] = groupValues[sourceOffset + valueIndex] ? uint8{1} : uint8{0};
      }
    }
    else
    {
      std::memcpy(sliceBuffer.data() + destinationOffset * sizeof(T), groupValues.data() + sourceOffset, valueCount * sizeof(T));
    }
  };

  if(planeIndex == 0)
  {
    const usize sliceValues = dimX * dimY * numComponents;
    const usize rowValues = dimX * numComponents;
    for(usize row = 0; row < dimY; ++row)
    {
      copyValues(row * rowValues, localSliceIndex * sliceValues + row * rowValues, rowValues);
      if(reportRows)
      {
        reportRows(row + 1, dimY);
      }
    }
    return;
  }

  if(planeIndex == 1)
  {
    const usize rowValues = dimX * numComponents;
    for(usize zIndex = 0; zIndex < dimZ; ++zIndex)
    {
      const usize sourceOffset = (zIndex * groupSliceCount + localSliceIndex) * rowValues;
      copyValues(zIndex * rowValues, sourceOffset, rowValues);
      if(reportRows)
      {
        reportRows(zIndex + 1, dimZ);
      }
    }
    return;
  }

  for(usize zIndex = 0; zIndex < dimZ; ++zIndex)
  {
    for(usize yIndex = 0; yIndex < dimY; ++yIndex)
    {
      const usize sourceOffset = ((zIndex * dimY + yIndex) * groupSliceCount + localSliceIndex) * numComponents;
      const usize destinationOffset = (zIndex * dimY + yIndex) * numComponents;
      copyValues(destinationOffset, sourceOffset, numComponents);
    }
    if(reportRows)
    {
      reportRows(zIndex + 1, dimZ);
    }
  }
}

/**
 * @struct WriteDirectVolumeFunctor
 * @brief Reads bounded groups of typed slices into caller-owned storage and writes each output slice.
 */
struct WriteDirectVolumeFunctor
{
  /**
   * @brief Writes all direct-value slices for one source array.
   * @tparam T Specifies the source value type.
   * @param dataArray Source array.
   * @param planeIndex Selects the XY, XZ, or YZ plane.
   * @param dimX Number of source tuples on the X axis.
   * @param dimY Number of source tuples on the Y axis.
   * @param dimZ Number of source tuples on the Z axis.
   * @param sliceCount Number of output slices.
   * @param sliceW Output width in pixels.
   * @param sliceH Output height in pixels.
   * @param numComponents Number of interleaved components in each tuple.
   * @param dataArrayPath Identifies the source array.
   * @param shouldCancel Stops processing between output slices.
   * @param writeSlice Writes one packed output slice.
   * @param reportRows Optional synchronous callback for completed output rows.
   * @return Error from the image writer.
   */
  template <typename T>
  Result<> operator()(const IDataArray& dataArray, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize sliceCount, usize sliceW, usize sliceH, usize numComponents,
                      const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel, const std::function<Result<>(std::vector<uint8>&, usize)>& writeSlice,
                      const std::function<void(usize, usize)>& reportRows = {})
  {
    const auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize sliceValueCount = sliceW * sliceH * numComponents;
    const uint64 sliceBytes = static_cast<uint64>(sliceValueCount) * sizeof(T);
    const uint64 totalBytes = static_cast<uint64>(dataStore.getSize()) * sizeof(T);
    auto workingMemory = ImageProcessing::ReserveWorkingMemory(PreferredSliceGroupBytes(planeIndex, sliceBytes), totalBytes);
    const usize slicesPerGroup = CalculateSlicesPerGroup(planeIndex, sliceCount, sliceBytes, workingMemory.sizeBytes());
    if(slicesPerGroup == 0)
    {
      return MakeErrorResult(-27026,
                             fmt::format("Write Image cannot reserve one source slice for array '{}'. One slice requires {} bytes, but the working-memory manager granted {} bytes. Increase the cache "
                                         "memory budget or reduce the image slice dimensions.",
                                         dataArrayPath.toString(), sliceBytes, workingMemory.sizeBytes()));
    }

    std::unique_ptr<T[]> groupValues = std::make_unique<T[]>(slicesPerGroup * sliceValueCount);
    std::vector<uint8> sliceBuffer(sliceValueCount * (std::is_same_v<T, bool> ? 1 : sizeof(T)));

    for(usize firstSlice = 0; firstSlice < sliceCount; firstSlice += slicesPerGroup)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize groupSliceCount = std::min(slicesPerGroup, sliceCount - firstSlice);
      const usize groupValueCount = groupSliceCount * sliceValueCount;
      const Extent groupExtent = CreateSliceGroupExtent(planeIndex, firstSlice, groupSliceCount, dimX, dimY, dimZ);
      // A failed read leaves the group buffer holding stale or uninitialized values.
      // Stop here so the writer never commits those values as image pixels.
      if(Result<> result = dataStore.readExtentIntoBuffer(groupExtent, nonstd::span<T>(groupValues.get(), groupValueCount)); result.invalid())
      {
        return result;
      }

      for(usize localSliceIndex = 0; localSliceIndex < groupSliceCount; ++localSliceIndex)
      {
        if(shouldCancel)
        {
          return {};
        }
        CopySliceFromGroup(nonstd::span<const T>(groupValues.get(), groupValueCount), sliceBuffer, localSliceIndex, groupSliceCount, planeIndex, dimX, dimY, dimZ, numComponents, reportRows);
        if(Result<> result = writeSlice(sliceBuffer, firstSlice + localSliceIndex); result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  }
};

/**
 * @brief Returns the grouped-buffer index for one output pixel.
 * @param planeIndex Selects the XY, XZ, or YZ plane.
 * @param localSliceIndex Selects one slice in the group.
 * @param groupSliceCount Number of slices in the group.
 * @param row Zero-based output row.
 * @param column Zero-based output column.
 * @param sliceW Output width in pixels.
 * @param sliceH Output height in pixels.
 * @return Zero-based value index in the grouped source extent.
 */
usize GroupSliceValueIndex(usize planeIndex, usize localSliceIndex, usize groupSliceCount, usize row, usize column, usize sliceW, usize sliceH)
{
  if(planeIndex == 0)
  {
    return localSliceIndex * sliceW * sliceH + row * sliceW + column;
  }
  if(planeIndex == 1)
  {
    return (row * groupSliceCount + localSliceIndex) * sliceW + column;
  }
  return (row * sliceW + column) * groupSliceCount + localSliceIndex;
}

/**
 * @brief Colorizes and writes a scalar volume one slice at a time.
 *
 * The first pass reduces the global range through fixed pages. The second pass reads bounded scalar and mask groups and writes one RGB slice at a time.
 */
struct ColorizeVolumeFunctor
{
  /**
   * @brief Colorizes and writes one scalar volume.
   * @tparam T Specifies the scalar source type.
   * @param dataArrayRef Scalar source array.
   * @param planeIndex Selects the XY, XZ, or YZ plane.
   * @param dimX Number of source tuples on the X axis.
   * @param dimY Number of source tuples on the Y axis.
   * @param dimZ Number of source tuples on the Z axis.
   * @param sliceCount Number of output slices.
   * @param sliceW Output width in pixels.
   * @param sliceH Output height in pixels.
   * @param dataArrayPath Identifies the scalar source array.
   * @param binPoints Normalized color-control positions.
   * @param controlPoints Color-control values.
   * @param numControlColors Number of control colors.
   * @param maskArray Optional Boolean or uint8 mask array.
   * @param invalidColor RGB value for masked pixels.
   * @param shouldCancel Stops processing between output slices.
   * @param writeSlice Writes one packed RGB output slice.
   * @param reportRange Optional synchronous callback for completed range-scan values.
   * @param reportRows Optional synchronous callback for completed output rows.
   * @return Error from source I/O or the image writer.
   */
  template <typename T>
  Result<> operator()(const IDataArray& dataArrayRef, usize planeIndex, usize dimX, usize dimY, usize dimZ, usize sliceCount, usize sliceW, usize sliceH, const DataPath& dataArrayPath,
                      const std::vector<float32>& binPoints, const std::vector<float32>& controlPoints, usize numControlColors, const IDataArray* maskArray, const std::vector<uint8>& invalidColor,
                      const std::atomic_bool& shouldCancel, const std::function<Result<>(std::vector<uint8>&, usize)>& writeSlice, const std::function<void(usize, usize)>& reportRange = {},
                      const std::function<void(usize, usize)>& reportRows = {})
  {
    const auto& dataStore = dataArrayRef.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numTuples = dataStore.getNumberOfTuples();
    if(numTuples == 0)
    {
      return MakeErrorResult(-27020, "Input array is empty.");
    }

    // Global min/max over the whole array (mask ignored) — matches Create Color Map semantics.
    constexpr usize k_TargetPageBytes = 1024 * 1024;
    const usize pageElements = std::max<usize>(1, std::min(numTuples, k_TargetPageBytes / sizeof(T)));
    auto valuePage = std::make_unique<T[]>(pageElements);
    T arrayMin{};
    T arrayMax{};
    bool initialized = false;
    for(usize offset = 0; offset < numTuples; offset += pageElements)
    {
      if(shouldCancel)
      {
        return {};
      }
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
      if(reportRange)
      {
        reportRange(offset + count, numTuples);
      }
    }

    valuePage.reset();

    const usize pixelCount = sliceW * sliceH;
    const uint64 sourceSliceBytes = static_cast<uint64>(pixelCount) * sizeof(T);
    const uint64 maskSliceBytes = maskArray == nullptr ? 0 : static_cast<uint64>(pixelCount);
    const uint64 groupSliceBytes = sourceSliceBytes + maskSliceBytes;
    const uint64 usefulGroupBytes = static_cast<uint64>(sliceCount) * groupSliceBytes;
    auto workingMemory = ImageProcessing::ReserveWorkingMemory(PreferredSliceGroupBytes(planeIndex, groupSliceBytes), usefulGroupBytes);
    const usize slicesPerGroup = CalculateSlicesPerGroup(planeIndex, sliceCount, groupSliceBytes, workingMemory.sizeBytes());
    if(slicesPerGroup == 0)
    {
      return MakeErrorResult(-27026,
                             fmt::format("Write Image cannot reserve one source slice for array '{}'. The scalar and mask buffers require {} bytes, but the working-memory manager granted {} bytes. "
                                         "Increase the cache memory budget or reduce the image slice dimensions.",
                                         dataArrayPath.toString(), groupSliceBytes, workingMemory.sizeBytes()));
    }

    std::unique_ptr<T[]> groupValues = std::make_unique<T[]>(slicesPerGroup * pixelCount);
    std::unique_ptr<bool[]> booleanMaskValues;
    std::unique_ptr<uint8[]> uint8MaskValues;
    if(maskArray != nullptr)
    {
      if(maskArray->getDataType() == DataType::boolean)
      {
        booleanMaskValues = std::make_unique<bool[]>(slicesPerGroup * pixelCount);
      }
      else
      {
        uint8MaskValues = std::make_unique<uint8[]>(slicesPerGroup * pixelCount);
      }
    }

    std::vector<uint8> sliceBuffer(sliceW * sliceH * 3);

    for(usize firstSlice = 0; firstSlice < sliceCount; firstSlice += slicesPerGroup)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize groupSliceCount = std::min(slicesPerGroup, sliceCount - firstSlice);
      const usize groupValueCount = groupSliceCount * pixelCount;
      const Extent groupExtent = CreateSliceGroupExtent(planeIndex, firstSlice, groupSliceCount, dimX, dimY, dimZ);
      // A failed read leaves the group buffers holding stale or uninitialized values.
      // Stop here so the writer never commits those values as image pixels.
      if(Result<> result = dataStore.readExtentIntoBuffer(groupExtent, nonstd::span<T>(groupValues.get(), groupValueCount)); result.invalid())
      {
        return result;
      }
      if(booleanMaskValues != nullptr)
      {
        if(Result<> result = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>().readExtentIntoBuffer(groupExtent, nonstd::span<bool>(booleanMaskValues.get(), groupValueCount)); result.invalid())
        {
          return result;
        }
      }
      else if(uint8MaskValues != nullptr)
      {
        if(Result<> result = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>().readExtentIntoBuffer(groupExtent, nonstd::span<uint8>(uint8MaskValues.get(), groupValueCount)); result.invalid())
        {
          return result;
        }
      }

      for(usize localSliceIndex = 0; localSliceIndex < groupSliceCount; ++localSliceIndex)
      {
        if(shouldCancel)
        {
          return {};
        }

        for(usize row = 0; row < sliceH; ++row)
        {
          if(shouldCancel)
          {
            return {};
          }
          for(usize col = 0; col < sliceW; ++col)
          {
            const usize groupValueIndex = GroupSliceValueIndex(planeIndex, localSliceIndex, groupSliceCount, row, col, sliceW, sliceH);
            const usize dstPixel = row * sliceW + col;
            const usize dst = dstPixel * 3;
            const bool maskValue = booleanMaskValues != nullptr ? booleanMaskValues[groupValueIndex] : (uint8MaskValues == nullptr || uint8MaskValues[groupValueIndex] != 0);
            if(!maskValue)
            {
              sliceBuffer[dst + 0] = invalidColor[0];
              sliceBuffer[dst + 1] = invalidColor[1];
              sliceBuffer[dst + 2] = invalidColor[2];
              continue;
            }

            const T sourceValue = groupValues[groupValueIndex];
            const float32 nValue = ColorTableUtilities::NormalizeValue(sourceValue, arrayMin, arrayMax);
            const std::array<uint8, 3> rgb = ColorTableUtilities::ComputeRgbFromControlPoints(nValue, binPoints, controlPoints, numControlColors);
            sliceBuffer[dst + 0] = rgb[0];
            sliceBuffer[dst + 1] = rgb[1];
            sliceBuffer[dst + 2] = rgb[2];
          }
          if(reportRows)
          {
            reportRows(row + 1, sliceH);
          }
        }

        if(Result<> result = writeSlice(sliceBuffer, firstSlice + localSliceIndex); result.invalid())
        {
          return result;
        }
      }
    }
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
WriteImageScanline::WriteImageScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

// -----------------------------------------------------------------------------
WriteImageScanline::~WriteImageScanline() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteImageScanline::operator()()
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

  // ImageMetadata is invariant across slices; color-table and scale-bar modes always write 3-component uint8.
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

  // Shared per-slice writer: names the file, writes via the ImageIO layer, commits atomically.
  auto writeSlice = [&](std::vector<uint8>& sliceBuffer, usize slice) -> Result<> {
    // A single-slice volume writes exactly the user-specified file name; the index suffix is only
    // appended when multiple slices are produced.
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

    // Flip operates on the un-padded slice; the scale-bar band is appended afterwards so the
    // bar is always upright at the bottom of the written image.
    ApplyImageFlip(sliceBuffer, sliceW, sliceH, incomingComps * incomingTypeSize, m_InputValues.flipMode);

    std::vector<uint8>* writeBufferPtr = &sliceBuffer;
    std::vector<uint8> paddedBuffer;
    if(addScaleBar)
    {
      // Preflight guarantees uint8 input (or color-table RGB) when the scale bar is enabled.
      paddedBuffer = ConvertUInt8ToRgb(sliceBuffer, sliceW * sliceH, incomingComps);
      paddedBuffer.insert(paddedBuffer.end(), bandRgb.begin(), bandRgb.end());
      writeBufferPtr = &paddedBuffer;
    }

    if(sliceCount == 1)
    {
      m_MessageHandler.sendInfoMessage("Encoding image file");
    }
    auto writeResult = imageIO->writePixelData(atomicFile.tempFilePath(), *writeBufferPtr, metadata);
    if(writeResult.invalid())
    {
      return writeResult;
    }
    auto commitResult = atomicFile.commit();
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
    const usize numControlColors = controlPoints.size() / 4;

    const IDataArray* maskArrayPtr = nullptr;
    if(m_InputValues.useMask)
    {
      maskArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues.maskArrayPath);
    }

    return ExecuteDataFunction(ColorizeVolumeFunctor{}, dataType, imageArray, m_InputValues.planeIndex, dimX, dimY, dimZ, sliceCount, sliceW, sliceH, m_InputValues.imageDataArrayPath, binPoints,
                               controlPoints, numControlColors, maskArrayPtr, m_InputValues.invalidColor, m_ShouldCancel, std::function<Result<>(std::vector<uint8>&, usize)>(writeSlice), reportRange,
                               reportRows);
  }

  return ExecuteDataFunction(WriteDirectVolumeFunctor{}, dataType, imageArray, m_InputValues.planeIndex, dimX, dimY, dimZ, sliceCount, sliceW, sliceH, nComp, m_InputValues.imageDataArrayPath,
                             m_ShouldCancel, std::function<Result<>(std::vector<uint8>&, usize)>(writeSlice), reportRows);
}

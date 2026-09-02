#include "WriteImage.hpp"

#include "simplnx/Common/AtomicFile.hpp"
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

// Flips a packed row-major 2D image buffer in place. pixelStrideBytes = numComponents * bytesPerComponent.
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

// Converts a packed uint8 slice buffer with 1, 3 or 4 components per pixel into 3-component RGB.
// Grayscale replicates into all three channels; RGBA drops the alpha channel.
// Precondition: buffer holds uint8 components (1 byte each); preflight enforces this whenever the scale bar is enabled.
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

// Builds a per-index "is this voxel good?" predicate from an optional bool/uint8 mask array.
std::function<bool(usize)> MakeMaskPredicate(const IDataArray* maskArray)
{
  if(maskArray == nullptr)
  {
    return [](usize) { return true; };
  }
  if(maskArray->getDataType() == DataType::boolean)
  {
    const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<bool>>();
    return [&maskStore](usize i) { return maskStore[i]; };
  }
  const auto& maskStore = maskArray->getIDataStoreRefAs<AbstractDataStore<uint8>>();
  return [&maskStore](usize i) { return maskStore[i] != 0; };
}

struct ColorizeVolumeFunctor
{
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

    // Global min/max over the whole array (mask ignored) — matches Create Color Map semantics.
    T arrayMin = dataStore[0];
    T arrayMax = dataStore[0];
    for(usize i = 1; i < numTuples; i++)
    {
      arrayMin = std::min(arrayMin, dataStore[i]);
      arrayMax = std::max(arrayMax, dataStore[i]);
    }

    const std::function<bool(usize)> isGood = MakeMaskPredicate(maskArray);

    // RGB output: 3 uint8 components per pixel.
    std::vector<uint8> sliceBuffer(sliceW * sliceH * 3);

    for(usize slice = 0; slice < sliceCount; ++slice)
    {
      if(shouldCancel)
      {
        return {};
      }

      for(usize row = 0; row < sliceH; ++row)
      {
        for(usize col = 0; col < sliceW; ++col)
        {
          usize srcIndex = 0;
          usize dstPixel = 0;
          if(planeIndex == 0) // XY: slice=z, col=x, row=y
          {
            srcIndex = slice * dimY * dimX + row * dimX + col;
            dstPixel = row * sliceW + col;
          }
          else if(planeIndex == 1) // XZ: slice=y, col=x, row=z
          {
            srcIndex = row * dimY * dimX + slice * dimX + col;
            dstPixel = row * sliceW + col;
          }
          else // YZ: slice=x, col=y, row=z
          {
            srcIndex = row * dimY * dimX + col * dimX + slice;
            dstPixel = row * sliceW + col;
          }

          const usize dst = dstPixel * 3;
          if(!isGood(srcIndex))
          {
            sliceBuffer[dst + 0] = invalidColor[0];
            sliceBuffer[dst + 1] = invalidColor[1];
            sliceBuffer[dst + 2] = invalidColor[2];
            continue;
          }

          const float32 nValue = ColorTableUtilities::NormalizeValue(dataStore[srcIndex], arrayMin, arrayMax);
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

  // Loop-invariant: the incoming per-slice buffer's component count and component byte size are the
  // same for every slice, so compute them once here rather than inside writeSlice on every call.
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

  // Shared per-slice writer: names the file, writes via the ImageIO layer, commits atomically.
  auto writeSlice = [&](std::vector<uint8>& sliceBuffer, usize slice) -> Result<> {
    m_MessageHandler.sendInfoMessage(fmt::format("Writing slice {}/{}", slice + 1, sliceCount));
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
    // Each control color is 4 floats [A,R,G,B]; interpolation requires at least 2 colors (8 floats),
    // otherwise ComputeRgbFromControlPoints would read past the end of the control-point array.
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

  // Non-color path: extract typed slice bytes and write.
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

#include "ReadNrrdFile.hpp"

#include "ImageProcessing/utils/NrrdUtilities.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ProgressTupleStride = 1u << 18; // ~256k tuples

struct CropBounds
{
  usize xStart{0};
  usize xEnd{0};
  usize yStart{0};
  usize yEnd{0};
  usize zStart{0};
  usize zEnd{0};
};

// Mirrors Algorithms/ReadNIfTIFile.cpp ComputeCropBounds (error codes shifted to -3574x).
Result<CropBounds> ComputeCropBounds(const nx::core::nrrd::NrrdMetadata& md, const CropGeometryParameter::ValueType& opts)
{
  CropBounds b;
  b.xEnd = md.dimensions[0] - 1;
  b.yEnd = md.dimensions[1] - 1;
  b.zEnd = md.dimensions[2] - 1;

  using Type = CropGeometryParameter::CropValues::TypeEnum;
  if(opts.type == Type::NoCropping)
  {
    return {b};
  }

  if(opts.type == Type::VoxelSubvolume)
  {
    if(opts.cropX)
    {
      b.xStart = opts.xBoundVoxels[0];
      b.xEnd = opts.xBoundVoxels[1];
    }
    if(opts.cropY)
    {
      b.yStart = opts.yBoundVoxels[0];
      b.yEnd = opts.yBoundVoxels[1];
    }
    if(opts.cropZ)
    {
      b.zStart = opts.zBoundVoxels[0];
      b.zEnd = opts.zBoundVoxels[1];
    }
  }
  else // PhysicalSubvolume
  {
    // The shared crop planner clamps physical bounds and floors voxel indices in float64.
    // Execute repeats that conversion so allocated and written extents match.
    // Bounds below or above the volume map to the first or last voxel.
    const auto clampFloorIndex = [](float64 coord, float64 origin, float64 spacing, usize dim) -> usize {
      const float64 idx = std::floor((coord - origin) / spacing);
      if(idx <= 0.0)
      {
        return 0;
      }
      const auto maxIdx = static_cast<float64>(dim - 1);
      if(idx >= maxIdx)
      {
        return dim - 1;
      }
      return static_cast<usize>(idx);
    };
    // start = clamped floor of the min bound (voxel 0 if the bound is below the origin);
    // end   = clamped floor of the max bound (last voxel if the bound is above the far corner).
    const auto axisBounds = [&](bool crop, const FloatVec2Type& bound, float32 origin, float32 spacing, usize dim) -> std::pair<usize, usize> {
      if(!crop)
      {
        return {usize{0}, dim - 1};
      }
      const auto originD = static_cast<float64>(origin);
      const auto spacingD = static_cast<float64>(spacing);
      const float32 farCorner = origin + static_cast<float32>(dim) * spacing; // matches ImageGeom::getBoundingBoxf max point
      const usize start = (bound[0] >= origin) ? clampFloorIndex(static_cast<float64>(bound[0]), originD, spacingD, dim) : usize{0};
      const usize end = (bound[1] <= farCorner) ? clampFloorIndex(static_cast<float64>(bound[1]), originD, spacingD, dim) : (dim - 1);
      return {start, end};
    };

    const auto [xStart, xEnd] = axisBounds(opts.cropX, opts.xBoundPhysical, md.origin[0], md.spacing[0], md.dimensions[0]);
    const auto [yStart, yEnd] = axisBounds(opts.cropY, opts.yBoundPhysical, md.origin[1], md.spacing[1], md.dimensions[1]);
    const auto [zStart, zEnd] = axisBounds(opts.cropZ, opts.zBoundPhysical, md.origin[2], md.spacing[2], md.dimensions[2]);
    b.xStart = xStart;
    b.xEnd = xEnd;
    b.yStart = yStart;
    b.yEnd = yEnd;
    b.zStart = zStart;
    b.zEnd = zEnd;
  }

  if(b.xStart > b.xEnd || b.yStart > b.yEnd || b.zStart > b.zEnd)
  {
    return MakeErrorResult<CropBounds>(-35742, fmt::format("Invalid crop bounds: start ({}, {}, {}) must be <= end ({}, {}, {}).", b.xStart, b.yStart, b.zStart, b.xEnd, b.yEnd, b.zEnd));
  }
  if(b.xEnd >= md.dimensions[0] || b.yEnd >= md.dimensions[1] || b.zEnd >= md.dimensions[2])
  {
    return MakeErrorResult<CropBounds>(
        -35743, fmt::format("Crop end voxel ({}, {}, {}) exceeds NRRD volume extent ({}, {}, {}).", b.xEnd, b.yEnd, b.zEnd, md.dimensions[0] - 1, md.dimensions[1] - 1, md.dimensions[2] - 1));
  }

  return {b};
}

/**
 * @brief Scan-line streamer. Reads one source scan-line (srcNx * componentCount
 *        elements) per NrrdDataReader::readBytes; out-of-range z-slices and y-rows
 *        are read and discarded. In-range x-subranges are byteswapped (if needed)
 *        into a scratch buffer and bulk-written via CopyFromArray::CopyData.
 *        OutputT == the file's native type (NRRD has no read-time scaling).
 */
template <class T>
Result<> StreamCroppedVoxels(nx::core::nrrd::NrrdDataReader& reader, AbstractDataStore<T>& store, const nx::core::nrrd::NrrdMetadata& md, const CropBounds& b, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
{
  const usize srcNx = md.dimensions[0];
  const usize srcNy = md.dimensions[1];
  const usize srcNz = md.dimensions[2];
  const usize comp = md.componentCount;
  const bool byteSwap = md.byteSwapRequired;

  const usize srcScanlineElements = srcNx * comp;
  const usize srcScanlineBytes = srcScanlineElements * sizeof(T);
  std::vector<T> srcScanline(srcScanlineElements);

  const usize destNx = b.xEnd - b.xStart + 1;
  const usize destNy = b.yEnd - b.yStart + 1;
  const usize destNz = b.zEnd - b.zStart + 1;
  const usize destScanlineElements = destNx * comp;
  std::vector<T> destScanline(destScanlineElements);

  const usize totalDestTuples = destNx * destNy * destNz;
  usize destTupleOffset = 0;
  usize lastProgressTuples = 0;

  for(usize srcZ = 0; srcZ < srcNz; srcZ++)
  {
    if(shouldCancel)
    {
      return {};
    }
    // All in-range z-slices have already been written (destTupleOffset has reached
    // totalDestTuples), so stop instead of reading + discarding the remaining
    // slices. For gzip this also avoids inflating the rest of the volume.
    if(srcZ > b.zEnd)
    {
      break;
    }
    const bool zInRange = (srcZ >= b.zStart && srcZ <= b.zEnd);

    for(usize srcY = 0; srcY < srcNy; srcY++)
    {
      Result<> readResult = reader.readBytes(srcScanline.data(), srcScanlineBytes);
      if(readResult.invalid())
      {
        return readResult;
      }

      if(!zInRange)
      {
        continue;
      }
      const bool yInRange = (srcY >= b.yStart && srcY <= b.yEnd);
      if(!yInRange)
      {
        continue;
      }

      usize writeIdx = 0;
      for(usize srcX = b.xStart; srcX <= b.xEnd; srcX++)
      {
        const usize baseIdx = srcX * comp;
        for(usize c = 0; c < comp; c++)
        {
          T raw = srcScanline[baseIdx + c];
          if(byteSwap)
          {
            raw = nx::core::byteswap(raw);
          }
          destScanline[writeIdx++] = raw;
        }
      }

      Result<> copyResult = CopyFromArray::CopyData(destScanline, store, destTupleOffset, 0, destNx, comp);
      if(copyResult.invalid())
      {
        return copyResult;
      }
      destTupleOffset += destNx;
    }
    if(destTupleOffset - lastProgressTuples >= k_ProgressTupleStride || destTupleOffset == totalDestTuples)
    {
      const auto pct = static_cast<int32>((destTupleOffset * 100ULL) / std::max<usize>(1, totalDestTuples));
      messageHandler({IFilter::Message::Type::Info, fmt::format("{}% Complete", pct)});
      lastProgressTuples = destTupleOffset;
    }
  }
  return {};
}

} // namespace

namespace nx::core
{
ReadNrrdFile::ReadNrrdFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNrrdFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReadNrrdFile::~ReadNrrdFile() noexcept = default;

Result<> ReadNrrdFile::operator()()
{
  auto metadataResult = nrrd::ReadNrrdHeader(m_InputValues->InputFilePath);
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  const auto& md = metadataResult.value();

  auto boundsResult = ComputeCropBounds(md, m_InputValues->CroppingOptions);
  if(boundsResult.invalid())
  {
    return ConvertResult(std::move(boundsResult));
  }
  const CropBounds bounds = boundsResult.value();

  auto readerResult = nrrd::NrrdDataReader::Create(md);
  if(readerResult.invalid())
  {
    return ConvertResult(std::move(readerResult));
  }
  auto& reader = *readerResult.value();

  const DataPath dataArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->ImageDataArrayName);
  auto& dataArrayBase = m_DataStructure.getDataRefAs<IDataArray>(dataArrayPath);

  switch(dataArrayBase.getDataType())
  {
  case DataType::int8:
    return StreamCroppedVoxels<int8>(reader, m_DataStructure.getDataRefAs<DataArray<int8>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::uint8:
    return StreamCroppedVoxels<uint8>(reader, m_DataStructure.getDataRefAs<DataArray<uint8>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::int16:
    return StreamCroppedVoxels<int16>(reader, m_DataStructure.getDataRefAs<DataArray<int16>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::uint16:
    return StreamCroppedVoxels<uint16>(reader, m_DataStructure.getDataRefAs<DataArray<uint16>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::int32:
    return StreamCroppedVoxels<int32>(reader, m_DataStructure.getDataRefAs<DataArray<int32>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::uint32:
    return StreamCroppedVoxels<uint32>(reader, m_DataStructure.getDataRefAs<DataArray<uint32>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::int64:
    return StreamCroppedVoxels<int64>(reader, m_DataStructure.getDataRefAs<DataArray<int64>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::uint64:
    return StreamCroppedVoxels<uint64>(reader, m_DataStructure.getDataRefAs<DataArray<uint64>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::float32:
    return StreamCroppedVoxels<float32>(reader, m_DataStructure.getDataRefAs<DataArray<float32>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  case DataType::float64:
    return StreamCroppedVoxels<float64>(reader, m_DataStructure.getDataRefAs<DataArray<float64>>(dataArrayPath).getDataStoreRef(), md, bounds, m_ShouldCancel, m_MessageHandler);
  default:
    return MakeErrorResult(-35733, fmt::format("Internal error: unexpected output DataType {}", static_cast<int>(dataArrayBase.getDataType())));
  }
}

} // namespace nx::core

#include "ReadNrrdFile.hpp"

#include "NXImageProcessing/utils/NrrdUtilities.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
struct CropBounds
{
  usize x_start{0};
  usize x_end{0};
  usize y_start{0};
  usize y_end{0};
  usize z_start{0};
  usize z_end{0};
};

// Mirrors Algorithms/ReadNIfTIFile.cpp ComputeCropBounds (error codes shifted to -3574x).
Result<CropBounds> ComputeCropBounds(const nx::core::nrrd::NrrdMetadata& md, const CropGeometryParameter::ValueType& opts)
{
  CropBounds b;
  b.x_end = md.dimensions[0] - 1;
  b.y_end = md.dimensions[1] - 1;
  b.z_end = md.dimensions[2] - 1;

  using _Type = CropGeometryParameter::CropValues::TypeEnum;
  if(opts.type == _Type::NoCropping)
  {
    return {b};
  }

  if(opts.type == _Type::VoxelSubvolume)
  {
    if(opts.crop_x)
    {
      b.x_start = opts.x_bound_voxels[0];
      b.x_end = opts.x_bound_voxels[1];
    }
    if(opts.crop_y)
    {
      b.y_start = opts.y_bound_voxels[0];
      b.y_end = opts.y_bound_voxels[1];
    }
    if(opts.crop_z)
    {
      b.z_start = opts.z_bound_voxels[0];
      b.z_end = opts.z_bound_voxels[1];
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

    const auto [xStart, xEnd] = axisBounds(opts.crop_x, opts.x_bound_physical, md.origin[0], md.spacing[0], md.dimensions[0]);
    const auto [yStart, yEnd] = axisBounds(opts.crop_y, opts.y_bound_physical, md.origin[1], md.spacing[1], md.dimensions[1]);
    const auto [zStart, zEnd] = axisBounds(opts.crop_z, opts.z_bound_physical, md.origin[2], md.spacing[2], md.dimensions[2]);
    b.x_start = xStart;
    b.x_end = xEnd;
    b.y_start = yStart;
    b.y_end = yEnd;
    b.z_start = zStart;
    b.z_end = zEnd;
  }

  if(b.x_start > b.x_end || b.y_start > b.y_end || b.z_start > b.z_end)
  {
    return MakeErrorResult<CropBounds>(-35742, fmt::format("Invalid crop bounds: start ({}, {}, {}) must be <= end ({}, {}, {}).", b.x_start, b.y_start, b.z_start, b.x_end, b.y_end, b.z_end));
  }
  if(b.x_end >= md.dimensions[0] || b.y_end >= md.dimensions[1] || b.z_end >= md.dimensions[2])
  {
    return MakeErrorResult<CropBounds>(
        -35743, fmt::format("Crop end voxel ({}, {}, {}) exceeds NRRD volume extent ({}, {}, {}).", b.x_end, b.y_end, b.z_end, md.dimensions[0] - 1, md.dimensions[1] - 1, md.dimensions[2] - 1));
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
  const usize comp = md.component_count;
  const bool byteSwap = md.byte_swap_required;

  const usize srcScanlineElements = srcNx * comp;
  const usize srcScanlineBytes = srcScanlineElements * sizeof(T);
  std::vector<T> srcScanline(srcScanlineElements);

  const usize destNx = b.x_end - b.x_start + 1;
  const usize destScanlineElements = destNx * comp;
  std::vector<T> destScanline(destScanlineElements);

  usize destTupleOffset = 0;
  ThrottledMessageHandler progressThrottle(messageHandler);
  progressThrottle.reset((b.z_end + 1) * srcNy, "Reading source rows");
  messageHandler.sendInfoMessage("Reading source rows and copying the selected volume; rows outside the crop are read and discarded");

  for(usize srcZ = 0; srcZ < srcNz; srcZ++)
  {
    if(shouldCancel)
    {
      return {};
    }
    // All in-range z-slices have already been written (srcZ has passed
    // the end of the crop), so stop instead of reading + discarding the remaining
    // slices. For gzip this also avoids inflating the rest of the volume.
    if(srcZ > b.z_end)
    {
      break;
    }
    const bool zInRange = (srcZ >= b.z_start && srcZ <= b.z_end);

    for(usize srcY = 0; srcY < srcNy; srcY++)
    {
      if(shouldCancel)
      {
        return {};
      }
      Result<> readResult = reader.readBytes(srcScanline.data(), srcScanlineBytes);
      if(readResult.invalid())
      {
        return readResult;
      }

      if(!zInRange)
      {
        progressThrottle.updatePercent(srcZ * srcNy + srcY + 1);
        continue;
      }
      const bool yInRange = (srcY >= b.y_start && srcY <= b.y_end);
      if(!yInRange)
      {
        progressThrottle.updatePercent(srcZ * srcNy + srcY + 1);
        continue;
      }

      usize writeIdx = 0;
      for(usize srcX = b.x_start; srcX <= b.x_end; srcX++)
      {
        const usize baseIdx = srcX * comp;
        for(usize c = 0; c < comp; c++)
        {
          T raw = srcScanline[baseIdx + c];
          if(byteSwap)
          {
            raw = nx::core::Byteswap(raw);
          }
          destScanline[writeIdx++] = raw;
        }
      }

      Result<> copyResult = copy_from_array::CopyData(destScanline, store, destTupleOffset, 0, destNx, comp);
      if(copyResult.invalid())
      {
        return copyResult;
      }
      destTupleOffset += destNx;
      progressThrottle.updatePercent(srcZ * srcNy + srcY + 1);
    }
  }
  messageHandler.sendProgressPercent("Reading source rows", (b.z_end + 1) * srcNy, (b.z_end + 1) * srcNy);
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

Result<> ReadNrrdFile::operator()()
{
  auto metadataResult = nrrd::ReadNrrdHeader(m_InputValues->input_file_path);
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  auto md = std::move(metadataResult).value();
  FloatVec3Type origin{md.origin[0], md.origin[1], md.origin[2]};
  FloatVec3Type spacing{md.spacing[0], md.spacing[1], md.spacing[2]};
  if(auto result = image_io::ResolveCropGeometry(m_InputValues->input_file_path, m_InputValues->geometry_overrides, m_InputValues->processing, m_InputValues->center_origin,
                                                 SizeVec3Type{md.dimensions[0], md.dimensions[1], md.dimensions[2]}, origin, spacing);
     result.invalid())
  {
    return result;
  }
  for(usize axis = 0; axis < 3; ++axis)
  {
    md.origin[axis] = origin[axis];
    md.spacing[axis] = spacing[axis];
  }

  auto boundsResult = ComputeCropBounds(md, m_InputValues->cropping_options);
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

  const DataPath dataArrayPath = m_InputValues->image_geometry_path.createChildPath(m_InputValues->cell_attribute_matrix_name).createChildPath(m_InputValues->image_data_array_name);
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

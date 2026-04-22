#include "ReadNIfTIFile.hpp"

#include "SimplnxCore/utils/NiftiUtilities.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/format.h>
#include <zlib.h>

#include <algorithm>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ProgressStride = 1u << 22; // ~4M elements processed per progress message

struct CropBounds
{
  usize xStart{0};
  usize xEnd{0};
  usize yStart{0};
  usize yEnd{0};
  usize zStart{0};
  usize zEnd{0};
};

Result<CropBounds> ComputeCropBounds(const nx::core::nifti::NiftiMetadata& md, const CropGeometryParameter::ValueType& opts)
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
      b.xStart = static_cast<usize>(opts.xBoundVoxels[0]);
      b.xEnd = static_cast<usize>(opts.xBoundVoxels[1]);
    }
    if(opts.cropY)
    {
      b.yStart = static_cast<usize>(opts.yBoundVoxels[0]);
      b.yEnd = static_cast<usize>(opts.yBoundVoxels[1]);
    }
    if(opts.cropZ)
    {
      b.zStart = static_cast<usize>(opts.zBoundVoxels[0]);
      b.zEnd = static_cast<usize>(opts.zBoundVoxels[1]);
    }
  }
  else // PhysicalSubvolume
  {
    DataStructure tmpDs;
    auto* srcImageGeom = ImageGeom::Create(tmpDs, "srcImageGeom");
    srcImageGeom->setOrigin({md.origin[0], md.origin[1], md.origin[2]});
    srcImageGeom->setSpacing({md.spacing[0], md.spacing[1], md.spacing[2]});
    srcImageGeom->setDimensions({md.dimensions[0], md.dimensions[1], md.dimensions[2]});

    const auto startCoords = srcImageGeom->getCoordsf(b.xStart, b.yStart, b.zStart);
    const auto endCoords = srcImageGeom->getCoordsf(b.xEnd, b.yEnd, b.zEnd);

    FloatVec2Type xBoundPhysical = opts.cropX ? opts.xBoundPhysical : FloatVec2Type{startCoords[0], endCoords[0]};
    FloatVec2Type yBoundPhysical = opts.cropY ? opts.yBoundPhysical : FloatVec2Type{startCoords[1], endCoords[1]};
    FloatVec2Type zBoundPhysical = opts.cropZ ? opts.zBoundPhysical : FloatVec2Type{startCoords[2], endCoords[2]};

    auto startIndex = srcImageGeom->getIndex(xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]);
    if(!startIndex.has_value())
    {
      return MakeErrorResult<CropBounds>(
          -34740, fmt::format("Could not map starting physical bounds ({}, {}, {}) to a valid voxel index within the NIfTI volume.", xBoundPhysical[0], yBoundPhysical[0], zBoundPhysical[0]));
    }
    b.xStart = startIndex.value() % md.dimensions[0];
    b.yStart = (startIndex.value() / md.dimensions[0]) % md.dimensions[1];
    b.zStart = startIndex.value() / (md.dimensions[0] * md.dimensions[1]);

    auto endIndex = srcImageGeom->getIndex(xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]);
    if(!endIndex.has_value())
    {
      return MakeErrorResult<CropBounds>(
          -34741, fmt::format("Could not map ending physical bounds ({}, {}, {}) to a valid voxel index within the NIfTI volume.", xBoundPhysical[1], yBoundPhysical[1], zBoundPhysical[1]));
    }
    b.xEnd = endIndex.value() % md.dimensions[0];
    b.yEnd = (endIndex.value() / md.dimensions[0]) % md.dimensions[1];
    b.zEnd = endIndex.value() / (md.dimensions[0] * md.dimensions[1]);
  }

  if(b.xStart > b.xEnd || b.yStart > b.yEnd || b.zStart > b.zEnd)
  {
    return MakeErrorResult<CropBounds>(-34742, fmt::format("Invalid crop bounds: start ({}, {}, {}) must be <= end ({}, {}, {}).", b.xStart, b.yStart, b.zStart, b.xEnd, b.yEnd, b.zEnd));
  }
  if(b.xEnd >= md.dimensions[0] || b.yEnd >= md.dimensions[1] || b.zEnd >= md.dimensions[2])
  {
    return MakeErrorResult<CropBounds>(
        -34743, fmt::format("Crop end voxel ({}, {}, {}) exceeds NIfTI volume extent ({}, {}, {}).", b.xEnd, b.yEnd, b.zEnd, md.dimensions[0] - 1, md.dimensions[1] - 1, md.dimensions[2] - 1));
  }

  return {b};
}

template <class NativeT, class OutputT>
Result<> StreamCroppedVoxels(gzFile gz, AbstractDataStore<OutputT>& store, const nx::core::nifti::NiftiMetadata& md, const CropBounds& b, bool applyScaling, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
{
  static_assert(std::is_arithmetic_v<NativeT>, "StreamCroppedVoxels requires arithmetic native type");

  const usize srcNx = md.dimensions[0];
  const usize srcNy = md.dimensions[1];
  const usize srcNz = md.dimensions[2];
  const usize componentCount = md.componentCount;
  const bool byteSwap = md.byteSwapRequired;
  const float32 slope = md.sclSlope;
  const float32 inter = md.sclInter;
  const std::string& filePath = md.filePath;

  const usize scanlineElements = srcNx * componentCount;
  const usize scanlineBytes = scanlineElements * sizeof(NativeT);
  std::vector<NativeT> scanline(scanlineElements);

  const usize destNx = b.xEnd - b.xStart + 1;
  const usize destNy = b.yEnd - b.yStart + 1;
  const usize destNz = b.zEnd - b.zStart + 1;
  const usize totalDestElements = destNx * destNy * destNz * componentCount;

  usize destIdx = 0;
  usize lastProgressIdx = 0;

  for(usize srcZ = 0; srcZ < srcNz; srcZ++)
  {
    if(shouldCancel)
    {
      return {};
    }
    const bool zInRange = (srcZ >= b.zStart && srcZ <= b.zEnd);

    for(usize srcY = 0; srcY < srcNy; srcY++)
    {
      const int actuallyRead = gzread(gz, scanline.data(), static_cast<unsigned int>(scanlineBytes));
      if(actuallyRead != static_cast<int>(scanlineBytes))
      {
        return MakeErrorResult(-34730, fmt::format("Short voxel read from '{}' at (z={}, y={}): expected {} bytes, got {}", filePath, srcZ, srcY, scanlineBytes, actuallyRead));
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

      for(usize srcX = b.xStart; srcX <= b.xEnd; srcX++)
      {
        const usize baseIdx = srcX * componentCount;
        for(usize c = 0; c < componentCount; c++)
        {
          NativeT raw = scanline[baseIdx + c];
          if(byteSwap)
          {
            raw = nx::core::byteswap(raw);
          }

          OutputT outVal;
          if constexpr(std::is_same_v<OutputT, float32>)
          {
            const auto promoted = static_cast<float32>(raw);
            outVal = applyScaling ? (promoted * slope + inter) : promoted;
          }
          else
          {
            outVal = static_cast<OutputT>(raw);
          }
          store.setValue(destIdx++, outVal);
        }
      }

      if(destIdx - lastProgressIdx >= k_ProgressStride || destIdx == totalDestElements)
      {
        const auto pct = static_cast<int32>((destIdx * 100ULL) / std::max<usize>(1, totalDestElements));
        messageHandler({IFilter::Message::Type::Info, fmt::format("Reading NIfTI voxels... {}%", pct)});
        lastProgressIdx = destIdx;
      }
    }
  }
  return {};
}

template <class OutputT>
Result<> DispatchByNiftiType(gzFile gz, AbstractDataStore<OutputT>& store, const nx::core::nifti::NiftiMetadata& md, const CropBounds& b, bool applyScaling, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
{
  switch(md.niftiDatatype)
  {
  case NIFTI_TYPE_UINT8:
  case NIFTI_TYPE_RGB24:
  case NIFTI_TYPE_RGBA32:
    return StreamCroppedVoxels<uint8, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_INT8:
    return StreamCroppedVoxels<int8, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_UINT16:
    return StreamCroppedVoxels<uint16, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_INT16:
    return StreamCroppedVoxels<int16, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_UINT32:
    return StreamCroppedVoxels<uint32, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_INT32:
    return StreamCroppedVoxels<int32, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_UINT64:
    return StreamCroppedVoxels<uint64, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_INT64:
    return StreamCroppedVoxels<int64, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_FLOAT32:
    return StreamCroppedVoxels<float32, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  case NIFTI_TYPE_FLOAT64:
    return StreamCroppedVoxels<float64, OutputT>(gz, store, md, b, applyScaling, shouldCancel, messageHandler);
  default:
    return MakeErrorResult(-34731, fmt::format("Unsupported NIfTI datatype code {} encountered during voxel read (header validation should have rejected this).", md.niftiDatatype));
  }
}

} // namespace

namespace nx::core
{

// -----------------------------------------------------------------------------
ReadNIfTIFile::ReadNIfTIFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNIfTIFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadNIfTIFile::~ReadNIfTIFile() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadNIfTIFile::operator()()
{
  auto metadataResult = nifti::ReadNiftiHeader(m_InputValues->InputFilePath, m_InputValues->UseAffineIfPresent);
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

  const bool applyScaling = m_InputValues->ApplyScalingTransform && md.hasNontrivialScaling && md.componentCount == 1;

  gzFile gz = gzopen(m_InputValues->InputFilePath.string().c_str(), "rb");
  if(gz == nullptr)
  {
    return MakeErrorResult(-34720, fmt::format("Could not open NIfTI file for reading: '{}'", m_InputValues->InputFilePath.string()));
  }

  const z_off_t targetOffset = static_cast<z_off_t>(md.voxOffset);
  if(gzseek(gz, targetOffset, SEEK_SET) != targetOffset)
  {
    gzclose(gz);
    return MakeErrorResult(-34721, fmt::format("Failed to seek to voxel offset {} in '{}'", md.voxOffset, m_InputValues->InputFilePath.string()));
  }

  const DataPath dataArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->ImageDataArrayName);
  auto& dataArrayBase = m_DataStructure.getDataRefAs<IDataArray>(dataArrayPath);

  Result<> streamResult;
  switch(dataArrayBase.getDataType())
  {
  case DataType::uint8:
    streamResult = DispatchByNiftiType<uint8>(gz, m_DataStructure.getDataRefAs<DataArray<uint8>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int8:
    streamResult = DispatchByNiftiType<int8>(gz, m_DataStructure.getDataRefAs<DataArray<int8>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint16:
    streamResult = DispatchByNiftiType<uint16>(gz, m_DataStructure.getDataRefAs<DataArray<uint16>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int16:
    streamResult = DispatchByNiftiType<int16>(gz, m_DataStructure.getDataRefAs<DataArray<int16>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint32:
    streamResult = DispatchByNiftiType<uint32>(gz, m_DataStructure.getDataRefAs<DataArray<uint32>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int32:
    streamResult = DispatchByNiftiType<int32>(gz, m_DataStructure.getDataRefAs<DataArray<int32>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint64:
    streamResult = DispatchByNiftiType<uint64>(gz, m_DataStructure.getDataRefAs<DataArray<uint64>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int64:
    streamResult = DispatchByNiftiType<int64>(gz, m_DataStructure.getDataRefAs<DataArray<int64>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::float32:
    streamResult = DispatchByNiftiType<float32>(gz, m_DataStructure.getDataRefAs<DataArray<float32>>(dataArrayPath).getDataStoreRef(), md, bounds, applyScaling, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::float64:
    streamResult = DispatchByNiftiType<float64>(gz, m_DataStructure.getDataRefAs<DataArray<float64>>(dataArrayPath).getDataStoreRef(), md, bounds, false, m_ShouldCancel, m_MessageHandler);
    break;
  default:
    gzclose(gz);
    return MakeErrorResult(-34722, fmt::format("Internal error: unexpected output DataType {}", static_cast<int>(dataArrayBase.getDataType())));
  }

  gzclose(gz);
  return streamResult;
}

} // namespace nx::core

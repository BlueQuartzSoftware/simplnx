#include "ReadNIfTIFile.hpp"

#include "SimplnxCore/utils/NiftiUtilities.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include <fmt/format.h>
#include <zlib.h>

#include <algorithm>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkElementCount = 1u << 20; // ~1M elements per chunk
constexpr usize k_ProgressStride = 1u << 22;    // report progress every ~4M elements processed

template <class NativeT, class OutputT>
Result<> StreamTypedVoxels(gzFile gz, AbstractDataStore<OutputT>& store, usize totalElements, bool byteSwap, bool applyScaling, float32 slope, float32 inter, const std::atomic_bool& shouldCancel,
                           const IFilter::MessageHandler& messageHandler, const std::string& filePath)
{
  static_assert(std::is_arithmetic_v<NativeT>, "StreamTypedVoxels requires arithmetic native type");

  std::vector<NativeT> buffer(k_ChunkElementCount);
  usize processed = 0;
  usize nextProgress = k_ProgressStride;

  while(processed < totalElements)
  {
    if(shouldCancel)
    {
      return {};
    }

    const usize toRead = std::min<usize>(k_ChunkElementCount, totalElements - processed);
    const usize bytesToRead = toRead * sizeof(NativeT);
    const int actuallyRead = gzread(gz, buffer.data(), static_cast<unsigned int>(bytesToRead));
    if(actuallyRead != static_cast<int>(bytesToRead))
    {
      return MakeErrorResult(-34730, fmt::format("Short voxel read from '{}': expected {} bytes at element offset {}, got {}", filePath, bytesToRead, processed, actuallyRead));
    }

    for(usize i = 0; i < toRead; i++)
    {
      NativeT raw = buffer[i];
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

      store.setValue(processed + i, outVal);
    }

    processed += toRead;
    if(processed >= nextProgress || processed == totalElements)
    {
      const auto pct = static_cast<int32>((processed * 100ULL) / totalElements);
      messageHandler({IFilter::Message::Type::Info, fmt::format("Reading ... {}%)", pct)});
      nextProgress += k_ProgressStride;
    }
  }
  return {};
}

template <class OutputT>
Result<> DispatchByNiftiType(gzFile gz, AbstractDataStore<OutputT>& store, const nx::core::nifti::NiftiMetadata& md, usize totalElements, bool applyScaling, const std::atomic_bool& shouldCancel,
                             const IFilter::MessageHandler& messageHandler)
{
  const float32 slope = md.sclSlope;
  const float32 inter = md.sclInter;
  const bool bs = md.byteSwapRequired;
  const std::string& fp = md.filePath;

  switch(md.niftiDatatype)
  {
  case NIFTI_TYPE_UINT8:
  case NIFTI_TYPE_RGB24:
  case NIFTI_TYPE_RGBA32:
    return StreamTypedVoxels<uint8, OutputT>(gz, store, totalElements, false, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_INT8:
    return StreamTypedVoxels<int8, OutputT>(gz, store, totalElements, false, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_UINT16:
    return StreamTypedVoxels<uint16, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_INT16:
    return StreamTypedVoxels<int16, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_UINT32:
    return StreamTypedVoxels<uint32, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_INT32:
    return StreamTypedVoxels<int32, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_UINT64:
    return StreamTypedVoxels<uint64, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_INT64:
    return StreamTypedVoxels<int64, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_FLOAT32:
    return StreamTypedVoxels<float32, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
  case NIFTI_TYPE_FLOAT64:
    return StreamTypedVoxels<float64, OutputT>(gz, store, totalElements, bs, applyScaling, slope, inter, shouldCancel, messageHandler, fp);
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

  const usize numVoxels = md.dimensions[0] * md.dimensions[1] * md.dimensions[2];
  const usize totalElements = numVoxels * md.componentCount;

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
    streamResult = DispatchByNiftiType<uint8>(gz, m_DataStructure.getDataRefAs<DataArray<uint8>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int8:
    streamResult = DispatchByNiftiType<int8>(gz, m_DataStructure.getDataRefAs<DataArray<int8>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint16:
    streamResult = DispatchByNiftiType<uint16>(gz, m_DataStructure.getDataRefAs<DataArray<uint16>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int16:
    streamResult = DispatchByNiftiType<int16>(gz, m_DataStructure.getDataRefAs<DataArray<int16>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint32:
    streamResult = DispatchByNiftiType<uint32>(gz, m_DataStructure.getDataRefAs<DataArray<uint32>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int32:
    streamResult = DispatchByNiftiType<int32>(gz, m_DataStructure.getDataRefAs<DataArray<int32>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::uint64:
    streamResult = DispatchByNiftiType<uint64>(gz, m_DataStructure.getDataRefAs<DataArray<uint64>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::int64:
    streamResult = DispatchByNiftiType<int64>(gz, m_DataStructure.getDataRefAs<DataArray<int64>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::float32:
    streamResult =
        DispatchByNiftiType<float32>(gz, m_DataStructure.getDataRefAs<DataArray<float32>>(dataArrayPath).getDataStoreRef(), md, totalElements, applyScaling, m_ShouldCancel, m_MessageHandler);
    break;
  case DataType::float64:
    streamResult = DispatchByNiftiType<float64>(gz, m_DataStructure.getDataRefAs<DataArray<float64>>(dataArrayPath).getDataStoreRef(), md, totalElements, false, m_ShouldCancel, m_MessageHandler);
    break;
  default:
    gzclose(gz);
    return MakeErrorResult(-34722, fmt::format("Internal error: unexpected output DataType {}", static_cast<int>(dataArrayBase.getDataType())));
  }

  gzclose(gz);
  return streamResult;
}

} // namespace nx::core

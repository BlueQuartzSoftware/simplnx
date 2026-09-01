#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include "simplnx/Utilities/Parsing/HDF5/ChunkIndex.hpp"
#include "simplnx/Utilities/Parsing/HDF5/DeflateEligibility.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkLoop.hpp"
#ifndef _WIN32
#include "simplnx/Utilities/PositionalFileIO.hpp"
#endif

#include <H5Dpublic.h>
#include <H5Fpublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>

#include <fmt/core.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>

namespace nx::core::HDF5
{
namespace
{
constexpr usize k_IncompressibilityProbeBytes = 4 * 1024;
constexpr usize k_IncompressibilityProbeSegments = 4;
constexpr usize k_MaxPreparedBatchBytes = 64 * 1024 * 1024;
constexpr usize k_MaxPreparedBatchChunks = 64;

/**
 * @brief Reads raw metadata for one chunk while the HDF5 API lock is held.
 * @param datasetId Open HDF5 dataset identifier.
 * @param offset Full-rank chunk-origin offset.
 * @return Allocation and raw-storage metadata for offset.
 * @pre Support::ApiLock() is held by the caller.
 *
 * allocated is false when HDF5 has no stored chunk address or the query fails.
 */
ParallelChunkCodec::ChunkInfo peekChunkInfoAtOffsetUnlocked(hid_t datasetId, const hsize_t* offset)
{
  ParallelChunkCodec::ChunkInfo info;
  haddr_t address = HADDR_UNDEF;
  hsize_t storedSize = 0;
  const herr_t status = H5Dget_chunk_info_by_coord(datasetId, offset, &info.filterMask, &address, &storedSize);
  info.allocated = (status >= 0 && address != HADDR_UNDEF && storedSize != 0);
  info.storedAddress = static_cast<uint64>(address);
  info.storedSize = static_cast<uint64>(storedSize);
  return info;
}

/**
 * @brief Builds one full-rank chunk-origin offset and reads its metadata.
 * @param datasetId Open HDF5 dataset identifier.
 * @param bounds Tuple-space chunk extent.
 * @param componentRank Number of trailing component dimensions.
 * @return Allocation and raw-storage metadata for bounds.
 * @pre Support::ApiLock() is held by the caller.
 */
ParallelChunkCodec::ChunkInfo peekChunkInfoUnlocked(hid_t datasetId, const Extent& bounds, usize componentRank)
{
  const usize tupleDims = bounds.min.size();
  const usize fullRank = tupleDims + componentRank;
  std::vector<hsize_t> offset(fullRank, 0);
  for(usize d = 0; d < tupleDims; ++d)
  {
    offset[d] = static_cast<hsize_t>(bounds.min[d]);
  }

  return peekChunkInfoAtOffsetUnlocked(datasetId, offset.data());
}

/**
 * @brief Reads one chunk's metadata under Support::ApiLock().
 * @param datasetId Open HDF5 dataset identifier.
 * @param bounds Tuple-space chunk extent.
 * @param componentRank Number of trailing component dimensions.
 * @return Allocation and raw-storage metadata for bounds.
 */
ParallelChunkCodec::ChunkInfo peekChunkInfo(hid_t datasetId, const Extent& bounds, usize componentRank)
{
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return peekChunkInfoUnlocked(datasetId, bounds, componentRank);
}

usize product(const std::vector<uint64>& shape)
{
  return std::accumulate(shape.begin(), shape.end(), usize{1}, std::multiplies<usize>());
}

/**
 * @brief Records the first worker failure.
 * @param firstErrorOut Optional shared diagnostic destination.
 * @param errorMutex Mutex that guards firstErrorOut.
 * @param message Failure diagnostic to record.
 *
 * Concurrent workers retain only the first message. A null destination ignores
 * diagnostics.
 */
void recordFirstError(std::string* firstErrorOut, std::mutex& errorMutex, std::string message)
{
  if(firstErrorOut == nullptr)
  {
    return;
  }
  std::lock_guard<std::mutex> lock(errorMutex);
  if(firstErrorOut->empty())
  {
    *firstErrorOut = std::move(message);
  }
}

/**
 * @brief Tests a distributed sample for likely incompressibility.
 * @param nominalBytes Full padded chunk bytes.
 * @param deflateLevel Dataset deflate level.
 * @return True when the sampled deflate result does not shrink.
 *
 * The probe avoids full-chunk compression for clearly incompressible data.
 */
bool isLikelyIncompressible(nonstd::span<const std::byte> nominalBytes, int32 deflateLevel)
{
  if(nominalBytes.size() <= k_IncompressibilityProbeBytes)
  {
    return false;
  }

  constexpr usize k_SegmentBytes = k_IncompressibilityProbeBytes / k_IncompressibilityProbeSegments;
  std::array<std::byte, k_IncompressibilityProbeBytes> sample{};
  const usize maxSourceOffset = nominalBytes.size() - k_SegmentBytes;
  const usize sourceStride = maxSourceOffset / (k_IncompressibilityProbeSegments - 1);
  for(usize segment = 0; segment < k_IncompressibilityProbeSegments; ++segment)
  {
    const usize sourceOffset = segment + 1 == k_IncompressibilityProbeSegments ? maxSourceOffset : segment * sourceStride;
    std::memcpy(sample.data() + segment * k_SegmentBytes, nominalBytes.data() + sourceOffset, k_SegmentBytes);
  }

  const uLong sourceLength = static_cast<uLong>(sample.size());
  uLongf compressedLength = compressBound(sourceLength);
  std::vector<std::byte> compressed(static_cast<usize>(compressedLength));
  const int result = compress2(reinterpret_cast<Bytef*>(compressed.data()), &compressedLength, reinterpret_cast<const Bytef*>(sample.data()), sourceLength, deflateLevel);
  return result == Z_OK && compressedLength >= sourceLength;
}
} // namespace

ParallelChunkCodec::ParallelChunkCodec(std::filesystem::path filePath, std::string datasetPath, std::vector<uint64> tupleShape, std::vector<uint64> chunkShape, std::vector<uint64> componentShape,
                                       usize elementSize, hid_t datasetId)
: m_FilePath(std::move(filePath))
, m_DatasetPath(std::move(datasetPath))
, m_TupleShape(std::move(tupleShape))
, m_ChunkShape(std::move(chunkShape))
, m_ComponentShape(std::move(componentShape))
, m_ElementSize(elementSize)
, m_DatasetId(datasetId)
{
  m_NumComponents = product(m_ComponentShape);
  m_NominalChunkElements = product(m_ChunkShape) * m_NumComponents;
  m_NumChunks = getNumberOfChunks(m_TupleShape, m_ChunkShape);
  // Probe once for single-deflate eligibility and capture the write deflate level.
  m_Eligible = probeSingleDeflateEligibility(m_DatasetId, m_ElementSize, &m_DeflateLevel);
}

ParallelChunkCodec::~ParallelChunkCodec()
{
#ifndef _WIN32
  if(detail::isValidFileHandle(m_ReadFileHandle))
  {
    detail::closeFileHandle(m_ReadFileHandle);
  }
#endif
}

#ifndef _WIN32
detail::FileHandle ParallelChunkCodec::getPositionalReadHandle() const
{
  std::call_once(m_ReadHandleOpenOnce, [this]() {
    m_ReadFileHandle = detail::openFileForRead(m_FilePath.string());
    if(!detail::isValidFileHandle(m_ReadFileHandle))
    {
      throw std::runtime_error(fmt::format("ParallelChunkCodec: failed to open '{}'", m_FilePath.string()));
    }
  });
  return m_ReadFileHandle;
}
#endif

bool ParallelChunkCodec::isEligible() const
{
  return m_Eligible;
}

std::vector<std::byte> ParallelChunkCodec::inflateChunk(uint64 flatChunkIndex) const
{
  const Extent bounds = getChunkBounds(flatChunkIndex, m_TupleShape, m_ChunkShape);

  // Reuse one metadata snapshot so this read performs one HDF5 chunk query.
  const ChunkInfo info = peekChunkInfo(m_DatasetId, bounds, m_ComponentShape.size());
  if(!info.allocated)
  {
    // A sparse chunk has no raw bytes. Best-effort prewarm can skip this distinct error.
    throw UnallocatedChunkError(fmt::format("ParallelChunkCodec: chunk {} not allocated (sparse / fill-value region) in '{}:{}'", flatChunkIndex, m_FilePath.string(), m_DatasetPath));
  }
  return inflateChunk(flatChunkIndex, info);
}

std::vector<ParallelChunkCodec::ChunkInfo> ParallelChunkCodec::getChunkInfos(nonstd::span<const uint64> flatChunkIndices) const
{
  std::vector<ChunkInfo> chunkInfos;
  chunkInfos.reserve(flatChunkIndices.size());
  const std::vector<uint64> chunksPerDimension = getChunksPerDimension(m_TupleShape, m_ChunkShape);
  std::vector<hsize_t> offset(m_TupleShape.size() + m_ComponentShape.size(), 0);
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  for(uint64 flatChunkIndex : flatChunkIndices)
  {
    uint64 remaining = flatChunkIndex;
    for(usize reverseDimension = m_TupleShape.size(); reverseDimension > 0; --reverseDimension)
    {
      const usize dimension = reverseDimension - 1;
      const uint64 chunkCoordinate = remaining % chunksPerDimension[dimension];
      remaining /= chunksPerDimension[dimension];
      offset[dimension] = static_cast<hsize_t>(chunkCoordinate * m_ChunkShape[dimension]);
    }
    chunkInfos.push_back(peekChunkInfoAtOffsetUnlocked(m_DatasetId, offset.data()));
  }
  return chunkInfos;
}

std::vector<std::byte> ParallelChunkCodec::inflateChunk(uint64 flatChunkIndex, const ChunkInfo& chunkInfo) const
{
  if(!chunkInfo.allocated)
  {
    throw UnallocatedChunkError(fmt::format("ParallelChunkCodec: chunk {} not allocated (sparse / fill-value region) in '{}:{}'", flatChunkIndex, m_FilePath.string(), m_DatasetPath));
  }
  const Extent bounds = getChunkBounds(flatChunkIndex, m_TupleShape, m_ChunkShape);
  return inflateChunkFromInfo(flatChunkIndex, bounds, chunkInfo.storedAddress, chunkInfo.storedSize, chunkInfo.filterMask);
}

std::vector<std::byte> ParallelChunkCodec::inflateChunkFromInfo(uint64 flatChunkIndex, const Extent& bounds, uint64 storedAddress, uint64 storedSize, uint32 filterMask) const
{
  const usize tupleDims = m_TupleShape.size();

  std::vector<std::byte> stored(static_cast<usize>(storedSize));
#ifdef _WIN32
  // Windows HDF5 owns the file range lock. Read raw bytes through HDF5, then
  // release Support::ApiLock() before worker inflation.
  const usize fullRank = tupleDims + m_ComponentShape.size();
  std::vector<hsize_t> offset(fullRank, 0);
  for(usize dimension = 0; dimension < tupleDims; ++dimension)
  {
    offset[dimension] = static_cast<hsize_t>(bounds.min[dimension]);
  }
  uint32 ignoredReadFilterMask = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    if(H5Dread_chunk(m_DatasetId, H5P_DEFAULT, offset.data(), &ignoredReadFilterMask, stored.data()) < 0)
    {
      throw std::runtime_error(fmt::format("ParallelChunkCodec: H5Dread_chunk failed on chunk {} of '{}:{}'", flatChunkIndex, m_FilePath.string(), m_DatasetPath));
    }
  }
  // Keep the H5Dget_chunk_info_by_coord metadata snapshot. On Windows, H5Dread_chunk
  // can report a stale filter mask after a live writable chunk is rewritten.
  static_cast<void>(storedAddress);
#else
  // POSIX workers reuse one positional descriptor. Recovery flushes only after
  // a short read from a recently extended file.
  auto readStoredBytes = [&]() -> std::ptrdiff_t {
    const nx::core::detail::FileHandle rawHandle = getPositionalReadHandle();
    return nx::core::detail::positionalRead(rawHandle, stored.data(), static_cast<std::size_t>(storedSize), static_cast<uint64_t>(storedAddress));
  };

  std::ptrdiff_t got = readStoredBytes();
  if(got != static_cast<std::ptrdiff_t>(storedSize))
  {
    // Keep normal reads lock and flush free. A second read under this mutex can
    // observe another worker's recovery before H5Fflush().
    std::lock_guard<std::mutex> recoveryLock(m_PositionalReadRecoveryMutex);
    got = readStoredBytes();
    if(got != static_cast<std::ptrdiff_t>(storedSize))
    {
      {
        std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
        if(H5Fflush(m_DatasetId, H5F_SCOPE_LOCAL) < 0)
        {
          throw std::runtime_error(
              fmt::format("ParallelChunkCodec: H5Fflush failed while recovering a short positional read on chunk {} of '{}:{}'", flatChunkIndex, m_FilePath.string(), m_DatasetPath));
        }
      }
      got = readStoredBytes();
    }
  }
  if(got != static_cast<std::ptrdiff_t>(storedSize))
  {
    throw std::runtime_error(fmt::format("ParallelChunkCodec: short positional read on chunk {} of '{}' ({} of {} bytes)", flatChunkIndex, m_FilePath.string(), got, storedSize));
  }
#endif

  // Inflate or copy into the full padded chunk buffer.
  const usize nominalBytes = m_NominalChunkElements * m_ElementSize;
  std::vector<std::byte> nominal(nominalBytes);

  // Bit zero marks skipped deflate. Such a chunk stores raw nominal bytes.
  const bool deflateSkipped = (filterMask & 0x1u) != 0u;
  if(deflateSkipped)
  {
    if(static_cast<usize>(storedSize) != nominalBytes)
    {
      throw std::runtime_error(fmt::format("ParallelChunkCodec: uncompressed chunk {} size {} != nominal {} in '{}'", flatChunkIndex, storedSize, nominalBytes, m_FilePath.string()));
    }
    std::memcpy(nominal.data(), stored.data(), nominalBytes);
  }
  else
  {
    // LLP64 zlib lengths can be 32-bit. Reject nominal chunks that would truncate.
    if(nominalBytes > static_cast<usize>(std::numeric_limits<uLongf>::max()))
    {
      throw std::runtime_error(
          fmt::format("ParallelChunkCodec: nominal chunk {} size {} exceeds zlib's {}-byte limit in '{}'", flatChunkIndex, nominalBytes, std::numeric_limits<uLongf>::max(), m_FilePath.string()));
    }
    uLongf destLen = static_cast<uLongf>(nominalBytes);
    const int zret = uncompress(reinterpret_cast<Bytef*>(nominal.data()), &destLen, reinterpret_cast<const Bytef*>(stored.data()), static_cast<uLong>(storedSize));
    if(zret != Z_OK || destLen != static_cast<uLongf>(nominalBytes))
    {
      throw std::runtime_error(
          fmt::format("ParallelChunkCodec: zlib uncompress failed (ret={}, got {} of {} bytes) on chunk {} of '{}'", zret, destLen, nominalBytes, flatChunkIndex, m_FilePath.string()));
    }
  }
  // Interior chunks already match the clamped layout. Edge chunks extract the
  // in-bounds region for serial H5Dread parity.
  bool isInterior = true;
  std::vector<uint64> clampedTupleDims(tupleDims);
  for(usize d = 0; d < tupleDims; ++d)
  {
    clampedTupleDims[d] = bounds.max[d] - bounds.min[d] + 1;
    if(clampedTupleDims[d] != m_ChunkShape[d])
    {
      isInterior = false;
    }
  }
  if(isInterior)
  {
    return nominal;
  }

  const usize clampedTuples = product(clampedTupleDims);
  const usize compBytes = m_NumComponents * m_ElementSize;
  std::vector<std::byte> clamped(clampedTuples * compBytes);
  for(usize flatClamped = 0; flatClamped < clampedTuples; ++flatClamped)
  {
    const std::vector<uint64> nd = flatToNd(static_cast<uint64>(flatClamped), clampedTupleDims);
    const usize nominalTupleFlat = static_cast<usize>(ndToFlat(nd, m_ChunkShape));
    std::memcpy(clamped.data() + flatClamped * compBytes, nominal.data() + nominalTupleFlat * compBytes, compBytes);
  }
  return clamped;
}

void ParallelChunkCodec::inflateChunksIntoSpan(nonstd::span<std::byte> out, nonstd::span<const uint64> flatChunkIndices) const
{
  const usize chunkCount = flatChunkIndices.size();
  if(chunkCount == 0)
  {
    return;
  }

  const usize tupleDims = m_TupleShape.size();
  const usize compBytes = m_NumComponents * m_ElementSize;

  // Validate once before workers scatter without per-write bounds checks.
  const usize requiredBytes = product(m_TupleShape) * compBytes;
  if(out.size() < requiredBytes)
  {
    throw std::runtime_error(fmt::format("ParallelChunkCodec: output span too small for '{}:{}' ({} < {} bytes)", m_FilePath.string(), m_DatasetPath, out.size(), requiredBytes));
  }

  // Scatter clamped bytes into their natural full-dataset offsets. Merge contiguous
  // rows so full-slab chunks use bulk memcpy instead of per-tuple copies.
  auto scatter = [&](const Extent& bounds, const std::vector<std::byte>& clamped) {
    std::vector<uint64> clampedTupleDims(tupleDims);
    for(usize d = 0; d < tupleDims; ++d)
    {
      clampedTupleDims[d] = bounds.max[d] - bounds.min[d] + 1;
    }

    // Merge outer dimensions only when the next inner dimension spans the full
    // dataset. A partial inner dimension leaves an output gap.
    usize runTuples = clampedTupleDims[tupleDims - 1];
    usize firstRunDim = tupleDims - 1;
    for(usize d = tupleDims - 1; d-- > 0;)
    {
      if(clampedTupleDims[d + 1] != m_TupleShape[d + 1])
      {
        break;
      }
      runTuples *= clampedTupleDims[d];
      firstRunDim = d;
    }

    const usize runBytes = runTuples * compBytes;
    const std::vector<uint64> outerDims(clampedTupleDims.begin(), clampedTupleDims.begin() + firstRunDim);
    usize numRuns = 1;
    for(usize d = 0; d < firstRunDim; ++d)
    {
      numRuns *= clampedTupleDims[d];
    }

    std::vector<uint64> globalCoords(tupleDims);
    for(usize d = firstRunDim; d < tupleDims; ++d)
    {
      globalCoords[d] = bounds.min[d];
    }
    for(usize runIdx = 0; runIdx < numRuns; ++runIdx)
    {
      if(firstRunDim > 0)
      {
        const std::vector<uint64> outerNd = flatToNd(static_cast<uint64>(runIdx), outerDims);
        for(usize d = 0; d < firstRunDim; ++d)
        {
          globalCoords[d] = bounds.min[d] + outerNd[d];
        }
      }
      const usize globalFlat = static_cast<usize>(ndToFlat(globalCoords, m_TupleShape));
      std::memcpy(out.data() + globalFlat * compBytes, clamped.data() + runIdx * runBytes, runBytes);
    }
  };

  // Chunks without raw metadata need a fill-aware serial read. Throwing
  // UnallocatedChunkError would skip the sink and leave the output region unwritten.
  auto serialFillRead = [&](uint64 flatChunkIndex, const Extent& bounds) -> std::vector<std::byte> {
    std::vector<uint64> clampedTupleDims(tupleDims);
    for(usize d = 0; d < tupleDims; ++d)
    {
      clampedTupleDims[d] = bounds.max[d] - bounds.min[d] + 1;
    }
    const usize clampedTuples = product(clampedTupleDims);
    std::vector<std::byte> clamped(clampedTuples * compBytes);

    const usize fullRank = tupleDims + m_ComponentShape.size();
    std::vector<hsize_t> fileStart(fullRank, 0);
    std::vector<hsize_t> count(fullRank, 0);
    std::vector<hsize_t> memDims(fullRank, 0);
    for(usize d = 0; d < tupleDims; ++d)
    {
      fileStart[d] = static_cast<hsize_t>(bounds.min[d]);
      count[d] = static_cast<hsize_t>(clampedTupleDims[d]);
      memDims[d] = static_cast<hsize_t>(clampedTupleDims[d]);
    }
    for(usize d = 0; d < m_ComponentShape.size(); ++d)
    {
      fileStart[tupleDims + d] = 0;
      count[tupleDims + d] = static_cast<hsize_t>(m_ComponentShape[d]);
      memDims[tupleDims + d] = static_cast<hsize_t>(m_ComponentShape[d]);
    }

    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      const hid_t fileSpace = H5Dget_space(m_DatasetId);
      const hid_t memSpace = H5Screate_simple(static_cast<int>(fullRank), memDims.data(), nullptr);
      const hid_t dtype = H5Dget_type(m_DatasetId);
      const herr_t selStatus = (fileSpace < 0) ? static_cast<herr_t>(-1) : H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, fileStart.data(), nullptr, count.data(), nullptr);
      const herr_t readStatus = (fileSpace < 0 || memSpace < 0 || dtype < 0 || selStatus < 0) ? static_cast<herr_t>(-1) : H5Dread(m_DatasetId, dtype, memSpace, fileSpace, H5P_DEFAULT, clamped.data());
      if(dtype >= 0)
      {
        H5Tclose(dtype);
      }
      if(memSpace >= 0)
      {
        H5Sclose(memSpace);
      }
      if(fileSpace >= 0)
      {
        H5Sclose(fileSpace);
      }
      // Report the first failed HDF5 call for a useful fallback-read diagnostic.
      const char* failedCall = nullptr;
      if(fileSpace < 0)
      {
        failedCall = "H5Dget_space";
      }
      else if(memSpace < 0)
      {
        failedCall = "H5Screate_simple";
      }
      else if(dtype < 0)
      {
        failedCall = "H5Dget_type";
      }
      else if(selStatus < 0)
      {
        failedCall = "H5Sselect_hyperslab";
      }
      else if(readStatus < 0)
      {
        failedCall = "H5Dread";
      }
      if(failedCall != nullptr)
      {
        throw std::runtime_error(fmt::format("ParallelChunkCodec: serial fill-value read failed at {} on chunk {} of '{}:{}'", failedCall, flatChunkIndex, m_FilePath.string(), m_DatasetPath));
      }
    }
    return clamped;
  };

  // One metadata probe selects raw inflation or the fill-aware H5Dread fallback.
  // A false allocation state can represent sparse data or a metadata-query failure.
  auto loader = [&](uint64 idx) -> std::vector<std::byte> {
    const Extent bounds = getChunkBounds(idx, m_TupleShape, m_ChunkShape);
    const ChunkInfo info = peekChunkInfo(m_DatasetId, bounds, m_ComponentShape.size());
    if(info.allocated)
    {
      return inflateChunkFromInfo(idx, bounds, info.storedAddress, info.storedSize, info.filterMask);
    }
    return serialFillRead(idx, bounds);
  };

  // Distinct chunk indices give tasks disjoint scatter regions without an output lock.
  auto sink = [&](usize localIndex, std::vector<std::byte>&& bytes) {
    const Extent bounds = getChunkBounds(flatChunkIndices[localIndex], m_TupleShape, m_ChunkShape);
    scatter(bounds, bytes);
  };

  // The shared engine records the first worker failure and rethrows after tasks finish.
  ParallelLoadChunks<std::vector<std::byte>>(flatChunkIndices, loader, sink);
}

std::vector<std::byte> ParallelChunkCodec::gatherChunkBytes(uint64 flatChunkIndex, nonstd::span<const std::byte> source, uint64 sourceStartTuple) const
{
  const usize tupleDims = m_TupleShape.size();
  const Extent bounds = getChunkBounds(flatChunkIndex, m_TupleShape, m_ChunkShape);

  std::vector<uint64> clampedTupleDims(tupleDims);
  for(usize d = 0; d < tupleDims; ++d)
  {
    clampedTupleDims[d] = bounds.max[d] - bounds.min[d] + 1;
  }

  // Zero initialization provides edge-chunk padding for serial-read parity.
  const usize compBytes = m_NumComponents * m_ElementSize;
  std::vector<std::byte> nominal(m_NominalChunkElements * m_ElementSize);

  // Merge rows contiguous in both layouts. A partial inner dimension prevents
  // an outer merge and would otherwise degrade to per-tuple copies.
  usize runTuples = clampedTupleDims[tupleDims - 1];
  usize firstRunDim = tupleDims - 1;
  for(usize d = tupleDims - 1; d-- > 0;)
  {
    if(clampedTupleDims[d + 1] != m_TupleShape[d + 1] || clampedTupleDims[d + 1] != m_ChunkShape[d + 1])
    {
      break;
    }
    runTuples *= clampedTupleDims[d];
    firstRunDim = d;
  }

  const usize runBytes = runTuples * compBytes;
  const std::vector<uint64> outerDims(clampedTupleDims.begin(), clampedTupleDims.begin() + firstRunDim);
  uint64 numRuns = 1;
  for(usize d = 0; d < firstRunDim; ++d)
  {
    numRuns *= clampedTupleDims[d];
  }

  std::vector<uint64> localNd(tupleDims, 0);
  std::vector<uint64> globalNd(tupleDims, 0);
  for(usize d = firstRunDim; d < tupleDims; ++d)
  {
    globalNd[d] = bounds.min[d];
  }
  for(uint64 runIdx = 0; runIdx < numRuns; ++runIdx)
  {
    if(firstRunDim > 0)
    {
      const std::vector<uint64> outerNd = flatToNd(runIdx, outerDims);
      for(usize d = 0; d < firstRunDim; ++d)
      {
        localNd[d] = outerNd[d];
        globalNd[d] = bounds.min[d] + outerNd[d];
      }
    }
    const usize srcOffset = static_cast<usize>(ndToFlat(globalNd, m_TupleShape) - sourceStartTuple) * compBytes;
    const usize dstOffset = static_cast<usize>(ndToFlat(localNd, m_ChunkShape)) * compBytes;
    std::memcpy(nominal.data() + dstOffset, source.data() + srcOffset, runBytes);
  }
  return nominal;
}

std::vector<std::byte> ParallelChunkCodec::compressChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const
{
  // Compression stays off the HDF5 lock. Reject chunks that exceed zlib lengths.
  if(nominalBytes.size() > static_cast<usize>(std::numeric_limits<uLong>::max()))
  {
    recordFirstError(firstErrorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: nominal chunk {} size {} exceeds zlib's {}-byte limit in '{}:{}'", flatChunkIndex, nominalBytes.size(), std::numeric_limits<uLong>::max(),
                                 m_FilePath.string(), m_DatasetPath));
    return {};
  }
  const uLong srcLen = static_cast<uLong>(nominalBytes.size());
  uLongf destLen = compressBound(srcLen);
  // Allocate the zlib bound without initialization, then retain only the used prefix.
  std::unique_ptr<std::byte[]> compressed = std::unique_ptr<std::byte[]>(new std::byte[static_cast<usize>(destLen)]);
  const int zret = compress2(reinterpret_cast<Bytef*>(compressed.get()), &destLen, reinterpret_cast<const Bytef*>(nominalBytes.data()), srcLen, m_DeflateLevel);
  if(zret != Z_OK)
  {
    recordFirstError(firstErrorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: zlib compress2 failed (ret={}, level={}) on chunk {} of '{}:{}'", zret, m_DeflateLevel, flatChunkIndex, m_FilePath.string(), m_DatasetPath));
    return {};
  }
  return std::vector<std::byte>(compressed.get(), compressed.get() + static_cast<usize>(destLen));
}

ParallelChunkCodec::PreparedChunkBytes ParallelChunkCodec::prepareChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut,
                                                                                 std::mutex& errorMutex) const
{
  if(isLikelyIncompressible(nominalBytes, m_DeflateLevel))
  {
    return {{}, 0x1U};
  }

  std::vector<std::byte> compressed = compressChunkBytesImpl(flatChunkIndex, nominalBytes, firstErrorOut, errorMutex);
  if(compressed.empty())
  {
    return {};
  }
  if(compressed.size() >= nominalBytes.size())
  {
    return {{}, 0x1U};
  }
  return {std::move(compressed), 0};
}

bool ParallelChunkCodec::writeCompressedChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> storedBytes, uint32 filterMask, std::string* firstErrorOut, std::mutex& errorMutex) const
{
  // Full-rank chunk origin appends component zeros because components remain whole.
  const usize tupleDims = m_TupleShape.size();
  const usize fullRank = tupleDims + m_ComponentShape.size();
  const Extent bounds = getChunkBounds(flatChunkIndex, m_TupleShape, m_ChunkShape);
  std::vector<hsize_t> offset(fullRank, 0);
  for(usize d = 0; d < tupleDims; ++d)
  {
    offset[d] = static_cast<hsize_t>(bounds.min[d]);
  }

  // Bit zero marks raw storage. Support::ApiLock() guards only the leaf write.
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    if(H5Dwrite_chunk(m_DatasetId, H5P_DEFAULT, filterMask, offset.data(), storedBytes.size(), storedBytes.data()) >= 0)
    {
      return true;
    }
  }
  recordFirstError(firstErrorOut, errorMutex,
                   fmt::format("ParallelChunkCodec: H5Dwrite_chunk failed ({} stored bytes, filter mask {}) on chunk {} of '{}:{}'", storedBytes.size(), filterMask, flatChunkIndex,
                               m_FilePath.string(), m_DatasetPath));
  return false;
}

bool ParallelChunkCodec::writeNominalChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const
{
  const PreparedChunkBytes prepared = prepareChunkBytesImpl(flatChunkIndex, nominalBytes, firstErrorOut, errorMutex);
  if(prepared.filterMask == 0 && prepared.filteredBytes.empty())
  {
    return false;
  }
  const nonstd::span<const std::byte> storedBytes = prepared.filterMask == 0 ? nonstd::span<const std::byte>(prepared.filteredBytes.data(), prepared.filteredBytes.size()) : nominalBytes;
  return writeCompressedChunkImpl(flatChunkIndex, storedBytes, prepared.filterMask, firstErrorOut, errorMutex);
}

std::vector<std::byte> ParallelChunkCodec::compressNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut) const
{
  // A private mutex keeps the shared first-error implementation uniform.
  if(errorOut != nullptr)
  {
    errorOut->clear();
  }
  std::mutex errorMutex;
  return compressChunkBytesImpl(flatChunkIndex, nominalBytes, errorOut, errorMutex);
}

bool ParallelChunkCodec::writeCompressedChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> compressedBytes, std::string* errorOut) const
{
  // A private mutex keeps the shared first-error implementation uniform.
  if(errorOut != nullptr)
  {
    errorOut->clear();
  }
  std::mutex errorMutex;
  return writeCompressedChunkImpl(flatChunkIndex, compressedBytes, 0, errorOut, errorMutex);
}

bool ParallelChunkCodec::writeNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut) const
{
  if(errorOut != nullptr)
  {
    errorOut->clear();
  }
  std::mutex errorMutex;
  if(!m_Eligible || flatChunkIndex >= m_NumChunks || nominalBytes.size() != m_NominalChunkElements * m_ElementSize)
  {
    recordFirstError(errorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: invalid nominal-chunk write for chunk {} of '{}:{}' (eligible={}, bytes={}, expected={}, chunks={})", flatChunkIndex, m_FilePath.string(),
                                 m_DatasetPath, m_Eligible, nominalBytes.size(), m_NominalChunkElements * m_ElementSize, m_NumChunks));
    return false;
  }
  return writeNominalChunkImpl(flatChunkIndex, nominalBytes, errorOut, errorMutex);
}

bool ParallelChunkCodec::writeRepeatedNominalChunk(nonstd::span<const std::byte> nominalBytes, nonstd::span<const uint64> flatChunkIndices, std::string* errorOut) const
{
  if(errorOut != nullptr)
  {
    errorOut->clear();
  }
  std::mutex errorMutex;
  if(!m_Eligible || nominalBytes.size() != m_NominalChunkElements * m_ElementSize)
  {
    recordFirstError(errorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: invalid repeated nominal-chunk write for '{}:{}' (eligible={}, bytes={}, expected={})", m_FilePath.string(), m_DatasetPath, m_Eligible,
                                 nominalBytes.size(), m_NominalChunkElements * m_ElementSize));
    return false;
  }
  if(flatChunkIndices.empty())
  {
    return true;
  }
  for(uint64 flatChunkIndex : flatChunkIndices)
  {
    if(flatChunkIndex >= m_NumChunks)
    {
      recordFirstError(errorOut, errorMutex,
                       fmt::format("ParallelChunkCodec: repeated nominal-chunk index {} is out of range ({} chunks) for '{}:{}'", flatChunkIndex, m_NumChunks, m_FilePath.string(), m_DatasetPath));
      return false;
    }
    const Extent bounds = getChunkBounds(flatChunkIndex, m_TupleShape, m_ChunkShape);
    for(usize dimension = 0; dimension < m_ChunkShape.size(); ++dimension)
    {
      if(bounds.max[dimension] - bounds.min[dimension] + 1 != m_ChunkShape[dimension])
      {
        recordFirstError(errorOut, errorMutex,
                         fmt::format("ParallelChunkCodec: repeated nominal-chunk index {} is clamped in dimension {} for '{}:{}'", flatChunkIndex, dimension, m_FilePath.string(), m_DatasetPath));
        return false;
      }
    }
  }

  const PreparedChunkBytes prepared = prepareChunkBytesImpl(flatChunkIndices.front(), nominalBytes, errorOut, errorMutex);
  if(prepared.filterMask == 0 && prepared.filteredBytes.empty())
  {
    return false;
  }
  const nonstd::span<const std::byte> storedBytes = prepared.filterMask == 0 ? nonstd::span<const std::byte>(prepared.filteredBytes.data(), prepared.filteredBytes.size()) : nominalBytes;
  for(uint64 flatChunkIndex : flatChunkIndices)
  {
    if(!writeCompressedChunkImpl(flatChunkIndex, storedBytes, prepared.filterMask, errorOut, errorMutex))
    {
      return false;
    }
  }
  return true;
}

bool ParallelChunkCodec::deflateSpanIntoChunks(nonstd::span<const std::byte> source, nonstd::span<const uint64> flatChunkIndices, uint64 sourceStartTuple, std::string* firstErrorOut) const
{
  std::mutex errorMutex;
  if(firstErrorOut != nullptr)
  {
    firstErrorOut->clear();
  }
  if(!m_Eligible)
  {
    recordFirstError(firstErrorOut, errorMutex, fmt::format("ParallelChunkCodec: dataset '{}:{}' is not eligible for the parallel deflate write path", m_FilePath.string(), m_DatasetPath));
    return false;
  }
  const usize chunkCount = flatChunkIndices.size();
  if(chunkCount == 0)
  {
    return true;
  }

  // Source must contain whole tuples within the dataset range.
  const uint64 totalTuples = static_cast<uint64>(product(m_TupleShape));
  const usize compBytes = m_NumComponents * m_ElementSize;
  if(source.size() % compBytes != 0)
  {
    recordFirstError(firstErrorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: source is {} bytes, not a whole number of {}-byte tuples, for '{}:{}'", source.size(), compBytes, m_FilePath.string(), m_DatasetPath));
    return false;
  }
  const uint64 sourceTupleCount = static_cast<uint64>(source.size() / compBytes);
  if(sourceStartTuple + sourceTupleCount > totalTuples)
  {
    recordFirstError(firstErrorOut, errorMutex,
                     fmt::format("ParallelChunkCodec: source tuple range [{}, {}) exceeds the {} tuples of '{}:{}'", sourceStartTuple, sourceStartTuple + sourceTupleCount, totalTuples,
                                 m_FilePath.string(), m_DatasetPath));
    return false;
  }

  // Validate indices and source containment before any batch commits.
  for(uint64 idx : flatChunkIndices)
  {
    if(idx >= m_NumChunks)
    {
      recordFirstError(firstErrorOut, errorMutex, fmt::format("ParallelChunkCodec: flat chunk index {} is out of range ({} chunks) for '{}:{}'", idx, m_NumChunks, m_FilePath.string(), m_DatasetPath));
      return false;
    }
    const Extent bounds = getChunkBounds(idx, m_TupleShape, m_ChunkShape);
    const uint64 firstTuple = ndToFlat(bounds.min, m_TupleShape);
    const uint64 lastTuple = ndToFlat(bounds.max, m_TupleShape);
    if(firstTuple < sourceStartTuple || lastTuple >= sourceStartTuple + sourceTupleCount)
    {
      recordFirstError(firstErrorOut, errorMutex,
                       fmt::format("ParallelChunkCodec: chunk {} of '{}:{}' covers flat tuples [{}, {}] but the source spans only [{}, {})", idx, m_FilePath.string(), m_DatasetPath, firstTuple,
                                   lastTuple, sourceStartTuple, sourceStartTuple + sourceTupleCount));
      return false;
    }
  }

  struct PendingChunkWrite
  {
    std::vector<std::byte> ownedBytes;
    const std::byte* borrowedData = nullptr;
    usize borrowedSize = 0;
    uint32 filterMask = 0;

    nonstd::span<const std::byte> storedBytes() const
    {
      if(!ownedBytes.empty())
      {
        return {ownedBytes.data(), ownedBytes.size()};
      }
      return {borrowedData, borrowedSize};
    }
  };

  // Limit preparation to 64 chunks. The 64 MiB target applies when one nominal
  // chunk is no larger than 64 MiB. A larger nominal chunk forms a one-chunk
  // batch. Commits stay serial because alternating H5Dwrite_chunk workers can corrupt adjacent chunks.
  const usize nominalChunkBytes = m_NominalChunkElements * m_ElementSize;
  const usize batchChunksByBytes = nominalChunkBytes == 0 ? 1 : std::max<usize>(1, k_MaxPreparedBatchBytes / nominalChunkBytes);
  const usize maxBatchChunks = std::min(k_MaxPreparedBatchChunks, batchChunksByBytes);

  for(usize batchStart = 0; batchStart < chunkCount; batchStart += maxBatchChunks)
  {
    const usize batchChunkCount = std::min(maxBatchChunks, chunkCount - batchStart);
    std::vector<PendingChunkWrite> pendingWrites(batchChunkCount);
    std::atomic<bool> ok{true};

    const auto prepareOne = [&](usize batchIndex) {
      if(!ok.load(std::memory_order_relaxed))
      {
        return;
      }
      const usize sourceIndex = batchStart + batchIndex;
      const uint64 idx = flatChunkIndices[sourceIndex];
      try
      {
        const Extent bounds = getChunkBounds(idx, m_TupleShape, m_ChunkShape);
        usize chunkTuples = 1;
        bool isFullNominalChunk = true;
        for(usize dimension = 0; dimension < m_TupleShape.size(); ++dimension)
        {
          const usize clampedDimension = static_cast<usize>(bounds.max[dimension] - bounds.min[dimension] + 1);
          chunkTuples *= clampedDimension;
          isFullNominalChunk = isFullNominalChunk && clampedDimension == m_ChunkShape[dimension];
        }
        const uint64 firstTuple = ndToFlat(bounds.min, m_TupleShape);
        const uint64 lastTuple = ndToFlat(bounds.max, m_TupleShape);
        const bool isContiguousSourceRun = lastTuple - firstTuple + 1 == chunkTuples;

        std::vector<std::byte> gathered;
        nonstd::span<const std::byte> nominal;
        if(isFullNominalChunk && isContiguousSourceRun)
        {
          const usize sourceByteOffset = static_cast<usize>(firstTuple - sourceStartTuple) * compBytes;
          const usize nominalBytes = chunkTuples * compBytes;
          nominal = source.subspan(sourceByteOffset, nominalBytes);
        }
        else
        {
          gathered = gatherChunkBytes(idx, source, sourceStartTuple);
          nominal = nonstd::span<const std::byte>(gathered.data(), gathered.size());
        }

        PreparedChunkBytes prepared = prepareChunkBytesImpl(idx, nominal, firstErrorOut, errorMutex);
        if(prepared.filterMask == 0 && prepared.filteredBytes.empty())
        {
          ok.store(false, std::memory_order_relaxed);
          return;
        }

        PendingChunkWrite& pending = pendingWrites[batchIndex];
        pending.filterMask = prepared.filterMask;
        if(prepared.filterMask == 0)
        {
          pending.ownedBytes = std::move(prepared.filteredBytes);
        }
        else if(!gathered.empty())
        {
          pending.ownedBytes = std::move(gathered);
        }
        else
        {
          pending.borrowedData = nominal.data();
          pending.borrowedSize = nominal.size();
        }
      } catch(...)
      {
        // Worker failures become a false result with the first diagnostic retained.
        recordFirstError(firstErrorOut, errorMutex, fmt::format("ParallelChunkCodec: exception while gathering/compressing chunk {} of '{}:{}'", idx, m_FilePath.string(), m_DatasetPath));
        ok.store(false, std::memory_order_relaxed);
      }
    };

    ParallelForChunkPositions(batchChunkCount, prepareOne);
    if(!ok.load(std::memory_order_relaxed))
    {
      return false;
    }

    for(usize batchIndex = 0; batchIndex < batchChunkCount; ++batchIndex)
    {
      const PendingChunkWrite& pending = pendingWrites[batchIndex];
      if(!writeCompressedChunkImpl(flatChunkIndices[batchStart + batchIndex], pending.storedBytes(), pending.filterMask, firstErrorOut, errorMutex))
      {
        return false;
      }
    }
  }

  return true;
}

} // namespace nx::core::HDF5

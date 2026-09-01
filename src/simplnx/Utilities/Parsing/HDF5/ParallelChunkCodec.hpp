#pragma once

#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/PositionalFileIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include <H5Ipublic.h>
#include <nonstd/span.hpp>

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace nx::core::HDF5
{

/**
 * @namespace nx::core::HDF5
 * @brief Contains HDF5 parsing utilities.
 */

/**
 * @class UnallocatedChunkError
 * @brief Signals that the codec cannot obtain raw chunk bytes.
 *
 * UnallocatedChunkError can represent an unallocated sparse chunk or a failed
 * HDF5 metadata query. The exception does not distinguish those causes.
 * An allocated-chunk raw-read or inflate failure uses std::runtime_error instead.
 */
class UnallocatedChunkError : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

/**
 * @class ParallelChunkCodec
 * @brief Reads and writes eligible deflate chunks with parallel off-lock work.
 *
 * The fast path requires one deflate filter and compatible host and file byte
 * order. Ineligible datasets use serial H5Dread fallback paths outside this class.
 * Direct raw reads bypass HDF5 conversion and filter handling.
 *
 * HDF5 metadata and raw-HDF5 calls hold Support::ApiLock(). POSIX raw reads use
 * shared positional I/O outside that lock. Windows uses H5Dread_chunk because
 * its HDF5 VFD holds the file range lock. zlib work remains outside the lock.
 *
 * Chunk tasks use the process-wide oneTBB scheduler when multicore support is
 * available. A serial loop preserves order when it is not. Operations wait for
 * scheduled work and do not accept cancellation.
 *
 * Read tasks preserve input chunk positions while they scatter disjoint byte
 * regions. Each inflate task owns one chunk buffer, so scheduler limits bound
 * concurrent inflation buffers. Deflate nominal preparation keeps at most 64
 * chunks in each batch. The 64 MiB target applies when one nominal chunk is no
 * larger than 64 MiB. A larger nominal chunk forms a one-chunk batch.
 * Batch commits stay serial because non-thread-safe HDF5 serializes writes.
 *
 * The caller must keep the HDF5 dataset handle valid and structurally unchanged
 * for the codec lifetime and each operation. The codec does not make generic
 * DataStore access safe for concurrent callers.
 */
class SIMPLNX_EXPORT ParallelChunkCodec
{
public:
  /**
   * @struct ChunkInfo
   * @brief Stores one raw-chunk metadata snapshot.
   */
  struct ChunkInfo
  {
    bool allocated = false;   // A false value can represent sparse storage or a failed HDF5 metadata query.
    uint32 filterMask = 0;    // HDF5 mask for filters skipped on this chunk.
    uint64 storedAddress = 0; // Raw chunk file offset.
    uint64 storedSize = 0;    // Raw chunk byte count.
  };

  /**
   * @brief Binds a codec to an open chunked dataset.
   * @param filePath Backing HDF5 file path for positional reads.
   * @param datasetPath HDF5 dataset path for diagnostics.
   * @param tupleShape Row-major tuple dimensions.
   * @param chunkShape Tuple-space chunk dimensions.
   * @param componentShape Trailing component dimensions that chunks do not split.
   * @param elementSize Bytes in one dataset element.
   * @param datasetId Open HDF5 dataset identifier that must outlive this codec.
   * @pre tupleShape and chunkShape have equal nonzero rank. All tuple and chunk
   * dimensions are nonzero.
   * @pre Each component dimension and elementSize are nonzero. An empty
   * componentShape represents one component.
   * @pre datasetId identifies an open HDF5 dataset.
   * @pre Component, nominal-chunk, and byte-count products fit usize. Each
   * tupleShape[d] + chunkShape[d] - 1 and the total chunk count fit uint64.
   *
   * The constructor probes eligibility once and captures the deflate level.
   */
  ParallelChunkCodec(std::filesystem::path filePath, std::string datasetPath, std::vector<uint64> tupleShape, std::vector<uint64> chunkShape, std::vector<uint64> componentShape, usize elementSize,
                     hid_t datasetId);

  /**
   * @brief Releases the positional read handle.
   *
   * The caller must finish all codec operations before destruction.
   */
  ~ParallelChunkCodec();

  ParallelChunkCodec(const ParallelChunkCodec&) = delete;
  ParallelChunkCodec(ParallelChunkCodec&&) = delete;
  ParallelChunkCodec& operator=(const ParallelChunkCodec&) = delete;
  ParallelChunkCodec& operator=(ParallelChunkCodec&&) = delete;

  /**
   * @brief Reports whether this dataset qualifies for raw parallel processing.
   * @return True for one deflate filter and compatible byte order.
   */
  bool isEligible() const;

  /**
   * @brief Inflates one chunk into an edge-clamped byte buffer.
   * @param flatChunkIndex Row-major logical chunk index.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws UnallocatedChunkError if metadata does not supply raw chunk bytes.
   * @throws std::runtime_error for an allocated chunk raw-read or inflate failure.
   * @pre isEligible() is true and flatChunkIndex is valid.
   *
   * One metadata probe supplies allocation, size, and filter state. POSIX reads
   * raw bytes through positional I/O. Windows uses H5Dread_chunk. Inflation and
   * edge clamping run outside Support::ApiLock().
   *
   * A false allocation state can identify an unallocated sparse chunk or a
   * failed HDF5 metadata query.
   */
  std::vector<std::byte> inflateChunk(uint64 flatChunkIndex) const;

  /**
   * @brief Locates chunks under one HDF5 API lock.
   * @param flatChunkIndices Row-major logical chunk indices.
   * @return Metadata snapshots aligned with flatChunkIndices positions.
   * @pre isEligible() is true and all indices are valid.
   * @pre The dataset remains structurally unchanged until matching inflates finish.
   *
   * Workers can pass these snapshots to inflateChunk() and avoid repeated
   * metadata calls. The output preserves input order. A false allocation state
   * can identify an unallocated sparse chunk or a failed HDF5 metadata query.
   */
  std::vector<ChunkInfo> getChunkInfos(nonstd::span<const uint64> flatChunkIndices) const;

  /**
   * @brief Inflates a chunk from a metadata snapshot.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param chunkInfo Metadata from getChunkInfos() for flatChunkIndex.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws UnallocatedChunkError if chunkInfo does not supply raw chunk bytes.
   * @throws std::runtime_error for a raw read or inflate failure.
   * @pre isEligible() is true and flatChunkIndex is valid.
   * @pre chunkInfo describes the same structurally unchanged dataset.
   *
   * A false allocation state can identify an unallocated sparse chunk or a
   * failed HDF5 metadata query.
   */
  std::vector<std::byte> inflateChunk(uint64 flatChunkIndex, const ChunkInfo& chunkInfo) const;

  /**
   * @brief Inflates selected chunks into a full-dataset byte span.
   * @param out Destination span for the full tuple and component layout.
   * @param flatChunkIndices Distinct row-major logical chunk indices.
   * @throws std::runtime_error if out is too small or a chunk read fails.
   * @pre isEligible() is true.
   * @pre flatChunkIndices contains distinct valid indices.
   * @pre Full-dataset tuple, component, and byte-count products fit usize. Flat
   * tuple indices fit uint64.
   *
   * Each task scatters one clamped chunk into a disjoint region of out. This
   * raw-byte contract does not make generic DataStore access concurrently safe.
   * Chunks without raw metadata use a serial fill-aware H5Dread under
   * Support::ApiLock().
   * Other worker failures are rethrown after scheduled tasks finish.
   */
  void inflateChunksIntoSpan(nonstd::span<std::byte> out, nonstd::span<const uint64> flatChunkIndices) const;

  /**
   * @brief Deflates selected chunks in bounded batches and commits them serially.
   * @param source Source tuple bytes beginning at sourceStartTuple.
   * @param flatChunkIndices Row-major logical chunk indices in desired commit order.
   * @param sourceStartTuple Global flat tuple index of source's first tuple.
   * @param firstErrorOut Optional first-failure diagnostic destination.
   * @return True when every requested chunk commits successfully.
   *
   * Worker tasks gather and deflate off the HDF5 lock. Each batch retains at
   * most 64 chunks. The 64 MiB target applies when one nominal chunk is no
   * larger than 64 MiB. A larger nominal chunk is prepared alone. The calling
   * thread commits prepared chunks in input order under leaf H5Dwrite_chunk locks.
   *
   * source remains caller-owned and must stay valid until this call returns.
   * Duplicate indices perform repeated serial commits in input order. The method
   * validates source containment and reports validation, zlib, or HDF5 errors
   * through firstErrorOut.
   * @pre Chunk, source, and full-dataset byte-count products fit usize. Flat
   * tuple indices fit uint64.
   * @pre sourceStartTuple plus the tuple count represented by source fits uint64.
   */
  bool deflateSpanIntoChunks(nonstd::span<const std::byte> source, nonstd::span<const uint64> flatChunkIndices, uint64 sourceStartTuple = 0, std::string* firstErrorOut = nullptr) const;

  /**
   * @brief Compresses one nominal chunk with zlib outside Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param errorOut Optional zlib failure diagnostic destination.
   * @return Compressed bytes, or an empty vector for a compression error.
   * @pre isEligible() is true.
   *
   * This is the serial entry point to the same compression worker implementation.
   */
  std::vector<std::byte> compressNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Writes pre-compressed bytes for one chunk under Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index.
   * @param compressedBytes Deflate bytes that use this dataset's filter stream.
   * @param errorOut Optional HDF5 failure diagnostic destination.
   * @return True on success; false when H5Dwrite_chunk fails.
   * @pre isEligible() is true and flatChunkIndex is valid.
   *
   * Component dimensions remain unsplit, so the full-rank offset appends zeros.
   * Filter mask zero records a deflate-compressed chunk.
   */
  bool writeCompressedChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> compressedBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Adaptively writes one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param nominalBytes Full padded chunk bytes.
   * @param errorOut Optional failure diagnostic destination.
   * @return True on success; false for invalid input, zlib, or HDF5 failure.
   *
   * A distributed trial avoids full compression for clearly incompressible data.
   * Raw chunks set filter-mask bit zero. Compressible chunks use the dataset
   * deflate stream. nominalBytes remains caller-owned during this call.
   */
  bool writeNominalChunk(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* errorOut = nullptr) const;

  /**
   * @brief Reuses one adaptive representation for full repeated chunks.
   * @param nominalBytes Full padded chunk bytes.
   * @param flatChunkIndices Full row-major logical chunk indices.
   * @param errorOut Optional failure diagnostic destination.
   * @return True when every chunk writes successfully.
   *
   * The method validates full chunk bounds. The trial and optional compression
   * run once before serial writes in input order.
   */
  bool writeRepeatedNominalChunk(nonstd::span<const std::byte> nominalBytes, nonstd::span<const uint64> flatChunkIndices, std::string* errorOut = nullptr) const;

private:
  /**
   * @struct PreparedChunkBytes
   * @brief Stores one adaptive raw or deflate representation.
   */
  struct PreparedChunkBytes
  {
    std::vector<std::byte> filteredBytes; // Deflate bytes when filterMask is zero.
    uint32 filterMask = 0;                // Bit zero indicates raw bytes bypass deflate.
  };

  /**
   * @brief Inflates one allocated chunk from raw storage metadata.
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param bounds Clamped tuple-space chunk extent.
   * @param storedAddress Raw chunk file offset.
   * @param storedSize Raw chunk byte count.
   * @param filterMask Chunk filter state.
   * @return In-bounds chunk bytes in row-major tuple and component order.
   * @throws std::runtime_error for short raw reads or inflate failure.
   *
   * POSIX raw reads use positional I/O. Windows uses H5Dread_chunk under the
   * HDF5 API lock. Deflate inflation and edge clamping remain outside the lock.
   */
  std::vector<std::byte> inflateChunkFromInfo(uint64 flatChunkIndex, const Extent& bounds, uint64 storedAddress, uint64 storedSize, uint32 filterMask) const;

  /**
   * @brief Gathers one source region into a padded nominal chunk buffer.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param source Tuple bytes beginning at sourceStartTuple.
   * @param sourceStartTuple Global flat tuple index of source's first tuple.
   * @return Full padded chunk bytes in row-major tuple and component order.
   *
   * Edge padding remains zero. The gather merges compatible contiguous rows to
   * avoid one memcpy per tuple.
   */
  std::vector<std::byte> gatherChunkBytes(uint64 flatChunkIndex, nonstd::span<const std::byte> source, uint64 sourceStartTuple) const;

  /**
   * @brief Compresses nominal bytes with zlib outside Support::ApiLock().
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return Compressed bytes, or an empty vector after a recorded failure.
   *
   * Parallel batches share one error destination. The public helper uses a
   * private destination and mutex.
   */
  std::vector<std::byte> compressChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Prepares raw or deflate bytes for one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index for diagnostics.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return Adaptive representation, or a default representation after failure.
   */
  PreparedChunkBytes prepareChunkBytesImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Prepares and writes one nominal chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param nominalBytes Full padded chunk bytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return True on successful adaptive write.
   */
  bool writeNominalChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> nominalBytes, std::string* firstErrorOut, std::mutex& errorMutex) const;

  /**
   * @brief Writes prepared raw bytes through H5Dwrite_chunk.
   * @param flatChunkIndex Row-major logical chunk index.
   * @param storedBytes Raw bytes to store.
   * @param filterMask HDF5 filter state for storedBytes.
   * @param firstErrorOut Shared first-failure destination, or null.
   * @param errorMutex Serializes firstErrorOut updates.
   * @return True when H5Dwrite_chunk succeeds.
   *
   * The full-rank offset appends component zeros. Support::ApiLock() guards
   * only this leaf write.
   */
  bool writeCompressedChunkImpl(uint64 flatChunkIndex, nonstd::span<const std::byte> storedBytes, uint32 filterMask, std::string* firstErrorOut, std::mutex& errorMutex) const;

#ifndef _WIN32
  /**
   * @brief Returns the lazy POSIX descriptor for positional reads.
   * @return Shared descriptor for positional reads.
   * @throws std::runtime_error if the backing file cannot open.
   *
   * pread does not change a shared file position, so workers reuse one descriptor.
   */
  detail::FileHandle getPositionalReadHandle() const;
#endif

  std::filesystem::path m_FilePath;
  std::string m_DatasetPath;
  std::vector<uint64> m_TupleShape;
  std::vector<uint64> m_ChunkShape;     // Tuple-space chunk dimensions.
  std::vector<uint64> m_ComponentShape; // Unsplittable trailing component dimensions.
  usize m_ElementSize = 1;
  hid_t m_DatasetId = H5I_INVALID_HID; // Borrowed HDF5 identifier; caller owns its lifetime.

  usize m_NumComponents = 1;        // Product of component dimensions.
  usize m_NominalChunkElements = 1; // Full padded tuple and component count.
  int32 m_DeflateLevel = 1;         // Eligibility-probe deflate level for compress2.
  uint64 m_NumChunks = 1;           // Total logical chunks for index validation.
  bool m_Eligible = false;

#ifndef _WIN32
  // Serializes short-read recovery only. Successful positional reads remain lock free.
  mutable std::mutex m_PositionalReadRecoveryMutex;
  mutable std::once_flag m_ReadHandleOpenOnce;
  mutable detail::FileHandle m_ReadFileHandle = detail::invalidFileHandle();
#endif
};

} // namespace nx::core::HDF5

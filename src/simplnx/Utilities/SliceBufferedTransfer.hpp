#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace nx::core
{

/**
 * @brief Type-dispatched functor that performs Z-slice buffered tuple transfer for a single DataArray.
 *
 * @section ooc_motivation OOC Motivation
 * Morphological fill algorithms (FillBadData, ErodeDilateCoordinationNumber, etc.) use a
 * "neighbors" mapping where each voxel stores the global index of the voxel it should copy
 * data from. When the underlying DataStore is out-of-core (OOC), accessing elements via
 * operator[] triggers chunk load/evict cycles -- one for the source read AND one for the
 * destination write -- for every single voxel. On large datasets this makes the algorithm
 * 100-1000x slower than in-memory.
 *
 * @section approach Approach: 3-Slot Rolling Window
 * Because face-neighbor morphological algorithms only ever copy from a source voxel that
 * is within +/-1 Z-slice of the destination, we can exploit this locality:
 *
 *   1. Maintain a rolling window of 3 source-slice buffers: slot 0 = z-1, slot 1 = z, slot 2 = z+1.
 *   2. For each Z-slice, bulk-read the destination slice into a local buffer.
 *   3. For each voxel in the slice that needs copying, look up the source from the
 *      appropriate slot (already in memory) and copy into the destination buffer.
 *   4. Bulk-write the modified destination slice back.
 *   5. Advance the rolling window: swap slots, read the next z+1 slice.
 *
 * This reduces OOC I/O from O(N) random element accesses to O(dimZ) sequential bulk reads/writes,
 * where each bulk operation transfers an entire slice in one copyIntoBuffer/copyFromBuffer call.
 *
 * @section memory Memory Footprint
 * 4 slices worth of element data: 3 source slots + 1 destination buffer.
 * For a 1024x1024 float32 array, that is approximately 16 MB -- trivial compared to
 * the full volume which could be many GB.
 *
 * @section bool_handling bool Specialization
 * std::vector<bool> is bit-packed and cannot provide a contiguous T* pointer, so
 * BufferType uses std::unique_ptr<bool[]> for bool and std::vector<T> for everything else.
 */
struct SliceBufferedTransferFunctor
{
  /**
   * @brief Buffer type alias that avoids std::vector<bool> bit-packing.
   *
   * std::vector<bool> is specialized to use 1 bit per element, which means
   * data() returns a proxy, not a bool*. The copyIntoBuffer/copyFromBuffer
   * API requires a contiguous T* span, so we use unique_ptr<bool[]> instead.
   *
   * @tparam T The element type of the DataArray being transferred.
   */
  template <typename T>
  using BufferType = std::conditional_t<std::is_same_v<T, bool>, std::unique_ptr<T[]>, std::vector<T>>;

  /**
   * @brief Returns a raw pointer to the underlying buffer data (vector overload).
   * @tparam T Element type.
   * @param v The vector buffer.
   * @return Pointer to the first element.
   */
  template <typename T>
  static T* bufPtr(std::vector<T>& v)
  {
    return v.data();
  }

  /**
   * @brief Returns a raw pointer to the underlying buffer data (unique_ptr overload).
   * @tparam T Element type.
   * @param p The unique_ptr buffer (used for bool specialization).
   * @return Pointer to the first element.
   */
  template <typename T>
  static T* bufPtr(std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

  /**
   * @brief Returns a const raw pointer to the underlying buffer data (const vector overload).
   * @tparam T Element type.
   * @param v The vector buffer.
   * @return Const pointer to the first element.
   */
  template <typename T>
  static const T* bufPtr(const std::vector<T>& v)
  {
    return v.data();
  }

  /**
   * @brief Returns a const raw pointer to the underlying buffer data (const unique_ptr overload).
   * @tparam T Element type.
   * @param p The unique_ptr buffer (used for bool specialization).
   * @return Const pointer to the first element.
   */
  template <typename T>
  static const T* bufPtr(const std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

  /**
   * @brief Factory method to create a buffer of the appropriate type for T.
   *
   * For bool, allocates a unique_ptr<bool[]> to avoid std::vector<bool> bit-packing.
   * For all other types, allocates a std::vector<T>.
   *
   * @tparam T Element type.
   * @param size Number of elements to allocate.
   * @return A BufferType<T> with the requested capacity.
   */
  template <typename T>
  static BufferType<T> makeBuf(usize size)
  {
    if constexpr(std::is_same_v<T, bool>)
    {
      return std::make_unique<T[]>(size);
    }
    else
    {
      return std::vector<T>(size);
    }
  }

  /**
   * @brief Performs the Z-slice buffered transfer for a typed DataArray.
   *
   * Iterates through all Z-slices, maintaining a 3-slot rolling source window.
   * For each destination voxel where neighbors[destIdx] >= 0 and shouldCopy(destIdx)
   * is true, copies the full tuple (all components) from the source location into
   * the destination buffer. Modified destination slices are written back via
   * copyFromBuffer.
   *
   * @tparam T The element type of the DataArray (dispatched by ExecuteDataFunction).
   * @param dataArray The DataArray to transfer data within (modified in place).
   * @param neighbors Global neighbor mapping: neighbors[i] is the global linear index
   *   of the source voxel for destination voxel i, or -1 if no copy is needed.
   * @param sliceSize Number of voxels per Z-slice (dimX * dimY).
   * @param dimZ Number of Z-slices in the volume.
   * @param shouldCopy Predicate returning true if the voxel at the given global index
   *   should be overwritten. Allows callers to skip voxels that are already valid.
   */
  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
  {
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    // sliceValues = number of T elements per Z-slice (voxels * components)
    const usize sliceValues = sliceSize * numComp;

    // Rolling source window: slot 0 = z-1, slot 1 = z, slot 2 = z+1
    // These three buffers are reused across all Z iterations via std::swap
    std::array<BufferType<T>, 3> srcSlices;
    for(auto& s : srcSlices)
    {
      s = makeBuf<T>(sliceValues);
    }
    auto destSlice = makeBuf<T>(sliceValues);

    // Lambda to bulk-read one Z-slice from the OOC store into a specific slot
    auto readSlice = [&](usize z, usize slot) { store.copyIntoBuffer(z * sliceValues, nonstd::span<T>(bufPtr(srcSlices[slot]), sliceValues)); };

    // Prime the rolling window: read z=0 into slot 1 (current), z=1 into slot 2 (next)
    readSlice(0, 1);
    if(dimZ > 1)
    {
      readSlice(1, 2);
    }

    for(usize zIdx = 0; zIdx < dimZ; zIdx++)
    {
      // Advance the rolling window by shifting slot contents down
      if(zIdx > 0)
      {
        std::swap(srcSlices[0], srcSlices[1]); // Old "current" becomes "previous"
        std::swap(srcSlices[1], srcSlices[2]); // Old "next" becomes "current"
        if(zIdx + 1 < dimZ)
        {
          readSlice(zIdx + 1, 2); // Read the new "next" slice
        }
      }

      // Bulk-read the destination slice so we can modify it in memory
      store.copyIntoBuffer(zIdx * sliceValues, nonstd::span<T>(bufPtr(destSlice), sliceValues));

      bool modified = false;
      for(usize inSlice = 0; inSlice < sliceSize; inSlice++)
      {
        const usize destIdx = zIdx * sliceSize + inSlice;
        const int64 srcIdx = neighbors[destIdx];
        if(srcIdx >= 0 && shouldCopy(destIdx))
        {
          // Determine which rolling-window slot contains the source data
          const usize srcZ = static_cast<usize>(srcIdx) / sliceSize;
          const usize srcInSlice = static_cast<usize>(srcIdx) % sliceSize;
          usize srcSlot = 1; // Same Z-slice (most common case)
          if(srcZ < zIdx)
          {
            srcSlot = 0; // Source is in previous Z-slice
          }
          else if(srcZ > zIdx)
          {
            srcSlot = 2; // Source is in next Z-slice
          }

          // Copy all components of the source tuple into the destination buffer
          for(usize c = 0; c < numComp; c++)
          {
            bufPtr(destSlice)[inSlice * numComp + c] = bufPtr(srcSlices[srcSlot])[srcInSlice * numComp + c];
          }
          modified = true;
        }
      }

      // Only write back if at least one tuple was modified (avoids unnecessary OOC writes)
      if(modified)
      {
        store.copyFromBuffer(zIdx * sliceValues, nonstd::span<const T>(bufPtr(destSlice), sliceValues));
      }
    }
  }
};

/**
 * @brief Convenience function to perform slice-buffered transfer on a single IDataArray.
 *
 * Dispatches on the array's DataType via ExecuteDataFunction to invoke the typed
 * SliceBufferedTransferFunctor::operator(). This is the primary entry point for
 * algorithms that have a full-volume neighbors mapping and want OOC-safe data transfer.
 *
 * @param dataArray The DataArray to transfer data within (modified in place).
 * @param neighbors Global neighbor mapping: neighbors[i] = source index for voxel i, or -1.
 * @param sliceSize Number of voxels per Z-slice (dimX * dimY).
 * @param dimZ Number of Z-slices.
 * @param shouldCopy Predicate: return true if the voxel at the given index should be overwritten.
 */
inline void SliceBufferedTransfer(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
{
  ExecuteDataFunction(SliceBufferedTransferFunctor{}, dataArray.getDataType(), dataArray, neighbors, sliceSize, dimZ, shouldCopy);
}

/**
 * @brief Type-dispatched functor that transfers a single Z-slice of a DataArray using per-slice marks.
 *
 * @section ooc_motivation OOC Motivation
 * SliceBufferedTransferFunctor processes the entire volume in one call and requires
 * a full-volume neighbors array. Some algorithms instead process one Z-slice at a time
 * (e.g., iterative morphological passes), building a per-slice marks array of size
 * sliceSize rather than a global neighbors array of size totalVoxels. This functor
 * supports that per-slice approach while maintaining OOC-safe bulk I/O.
 *
 * @section approach Approach: On-Demand Lazy Loading
 * For each voxel in the destination Z-slice where sliceMarks[inSlice] >= 0:
 *   1. Compute which Z-slice the source voxel is on (must be within +/-1 of destZ).
 *   2. Lazily load that source Z-slice if not already in memory.
 *   3. Copy the tuple from the source buffer into the destination buffer.
 *   4. Bulk-write the destination slice back when done.
 *
 * @section memory Memory Footprint
 * At most 4 slices: 1 destination + up to 3 source (z-1, z, z+1), each loaded on demand.
 */
struct SliceTransferOneZFunctor
{
  /**
   * @brief Transfers tuples for a single Z-slice based on per-slice marks.
   *
   * @tparam T The element type of the DataArray (dispatched by ExecuteDataFunction).
   * @param dataArray The DataArray to transfer data within (modified in place).
   * @param sliceMarks Per-slice marks of size sliceSize. sliceMarks[inSlice] is the global
   *   linear index of the source voxel, or -1 if no copy is needed for this position.
   * @param sliceSize Number of voxels per Z-slice (dimX * dimY).
   * @param destZ The Z-index of the destination slice being processed.
   * @param dimZ Total number of Z-slices (used for bounds checking).
   */
  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
  {
    using BufT = typename SliceBufferedTransferFunctor::BufferType<T>;
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    const usize sliceValues = sliceSize * numComp;

    // Bulk-read the destination slice into a local buffer
    auto destBuf = SliceBufferedTransferFunctor::makeBuf<T>(sliceValues);
    store.copyIntoBuffer(destZ * sliceValues, nonstd::span<T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));

    // Source slice buffers loaded lazily on demand: slot 0 = z-1, slot 1 = z, slot 2 = z+1
    std::array<BufT, 3> srcBufs;
    std::array<bool, 3> srcLoaded = {false, false, false};

    // Lazy-load helper: only reads a source Z-slice the first time it is needed
    auto ensureSrcLoaded = [&](usize slot, usize srcZ) {
      if(!srcLoaded[slot] && srcZ < dimZ)
      {
        srcBufs[slot] = SliceBufferedTransferFunctor::makeBuf<T>(sliceValues);
        store.copyIntoBuffer(srcZ * sliceValues, nonstd::span<T>(SliceBufferedTransferFunctor::bufPtr(srcBufs[slot]), sliceValues));
        srcLoaded[slot] = true;
      }
    };

    bool modified = false;
    for(usize inSlice = 0; inSlice < sliceSize; inSlice++)
    {
      const int64 srcGlobalIdx = sliceMarks[inSlice];
      if(srcGlobalIdx < 0)
      {
        continue; // No copy needed for this voxel
      }

      // Determine which Z-slice the source voxel is on and its in-slice offset
      const usize srcZ = static_cast<usize>(srcGlobalIdx) / sliceSize;
      const usize srcInSlice = static_cast<usize>(srcGlobalIdx) % sliceSize;

      // Map source Z to rolling-window slot: 0 = previous, 1 = same, 2 = next
      usize srcSlot = 1;
      if(srcZ < destZ)
      {
        srcSlot = 0;
      }
      else if(srcZ > destZ)
      {
        srcSlot = 2;
      }

      // Ensure the source slice is loaded before reading from it
      ensureSrcLoaded(srcSlot, srcZ);

      // Copy all components of the source tuple into the destination buffer
      for(usize c = 0; c < numComp; c++)
      {
        SliceBufferedTransferFunctor::bufPtr(destBuf)[inSlice * numComp + c] = SliceBufferedTransferFunctor::bufPtr(srcBufs[srcSlot])[srcInSlice * numComp + c];
      }
      modified = true;
    }

    // Only write back if at least one tuple was modified
    if(modified)
    {
      store.copyFromBuffer(destZ * sliceValues, nonstd::span<const T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));
    }
  }
};

/**
 * @brief Convenience function to transfer a single Z-slice of an IDataArray using per-slice marks.
 *
 * Dispatches on the array's DataType via ExecuteDataFunction to invoke the typed
 * SliceTransferOneZFunctor::operator(). This is the entry point for algorithms that
 * process one Z-slice at a time and maintain a per-slice marks array.
 *
 * @param dataArray The DataArray to transfer data within (modified in place).
 * @param sliceMarks Per-slice marks: sliceMarks[inSlice] = global source index, or -1.
 * @param sliceSize Number of voxels per Z-slice (dimX * dimY).
 * @param destZ The Z-index of the destination slice being processed.
 * @param dimZ Total number of Z-slices (for bounds checking).
 */
inline void SliceBufferedTransferOneZ(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
{
  ExecuteDataFunction(SliceTransferOneZFunctor{}, dataArray.getDataType(), dataArray, sliceMarks, sliceSize, destZ, dimZ);
}

} // namespace nx::core

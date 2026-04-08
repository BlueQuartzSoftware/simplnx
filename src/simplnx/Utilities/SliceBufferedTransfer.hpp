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
 * @brief Performs Z-slice buffered tuple transfer for a single typed DataArray.
 *
 * Reads source and destination Z-slices into memory, copies tuples in-memory
 * based on a neighbors mapping, then writes back. This eliminates per-element
 * OOC store access during transfer phases.
 *
 * Requires that source indices are always within ±1 Z-slice of the destination
 * (true for face-neighbor morphological algorithms).
 */
struct SliceBufferedTransferFunctor
{
  template <typename T>
  using BufferType = std::conditional_t<std::is_same_v<T, bool>, std::unique_ptr<T[]>, std::vector<T>>;

  template <typename T>
  static T* bufPtr(std::vector<T>& v)
  {
    return v.data();
  }

  template <typename T>
  static T* bufPtr(std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

  template <typename T>
  static const T* bufPtr(const std::vector<T>& v)
  {
    return v.data();
  }

  template <typename T>
  static const T* bufPtr(const std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

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

  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
  {
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    const usize sliceValues = sliceSize * numComp;

    // Rolling source window: slot 0=z-1, slot 1=z, slot 2=z+1
    std::array<BufferType<T>, 3> srcSlices;
    for(auto& s : srcSlices)
    {
      s = makeBuf<T>(sliceValues);
    }
    auto destSlice = makeBuf<T>(sliceValues);

    auto readSlice = [&](usize z, usize slot) { store.copyIntoBuffer(z * sliceValues, nonstd::span<T>(bufPtr(srcSlices[slot]), sliceValues)); };

    // Initialize source rolling window
    readSlice(0, 1);
    if(dimZ > 1)
    {
      readSlice(1, 2);
    }

    for(usize zIdx = 0; zIdx < dimZ; zIdx++)
    {
      if(zIdx > 0)
      {
        std::swap(srcSlices[0], srcSlices[1]);
        std::swap(srcSlices[1], srcSlices[2]);
        if(zIdx + 1 < dimZ)
        {
          readSlice(zIdx + 1, 2);
        }
      }

      // Read dest slice
      store.copyIntoBuffer(zIdx * sliceValues, nonstd::span<T>(bufPtr(destSlice), sliceValues));

      bool modified = false;
      for(usize inSlice = 0; inSlice < sliceSize; inSlice++)
      {
        const usize destIdx = zIdx * sliceSize + inSlice;
        const int64 srcIdx = neighbors[destIdx];
        if(srcIdx >= 0 && shouldCopy(destIdx))
        {
          const usize srcZ = static_cast<usize>(srcIdx) / sliceSize;
          const usize srcInSlice = static_cast<usize>(srcIdx) % sliceSize;
          usize srcSlot = 1;
          if(srcZ < zIdx)
          {
            srcSlot = 0;
          }
          else if(srcZ > zIdx)
          {
            srcSlot = 2;
          }

          for(usize c = 0; c < numComp; c++)
          {
            bufPtr(destSlice)[inSlice * numComp + c] = bufPtr(srcSlices[srcSlot])[srcInSlice * numComp + c];
          }
          modified = true;
        }
      }

      if(modified)
      {
        store.copyFromBuffer(zIdx * sliceValues, nonstd::span<const T>(bufPtr(destSlice), sliceValues));
      }
    }
  }
};

/**
 * @brief Convenience function to perform slice-buffered transfer on a single IDataArray.
 * Dispatches on the array's DataType to call the typed implementation.
 */
inline void SliceBufferedTransfer(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
{
  ExecuteDataFunction(SliceBufferedTransferFunctor{}, dataArray.getDataType(), dataArray, neighbors, sliceSize, dimZ, shouldCopy);
}

/**
 * @brief Transfers a single Z-slice of a typed DataArray using per-slice marks.
 *
 * For each voxel in the dest Z-slice where marks[inSlice] >= 0, copies the tuple
 * from the source location (which must be within ±1 Z of destZ). Manages its own
 * 3-slice source rolling window internally.
 *
 * This is the building block for O(sliceSize) memory algorithms that eliminate
 * full-volume neighbor arrays.
 */
struct SliceTransferOneZFunctor
{
  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
  {
    using BufT = typename SliceBufferedTransferFunctor::BufferType<T>;
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    const usize sliceValues = sliceSize * numComp;

    // Read dest slice
    auto destBuf = SliceBufferedTransferFunctor::makeBuf<T>(sliceValues);
    store.copyIntoBuffer(destZ * sliceValues, nonstd::span<T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));

    // Read source slices on demand (only those needed)
    std::array<BufT, 3> srcBufs; // 0=z-1, 1=z, 2=z+1
    std::array<bool, 3> srcLoaded = {false, false, false};

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
        continue;
      }

      const usize srcZ = static_cast<usize>(srcGlobalIdx) / sliceSize;
      const usize srcInSlice = static_cast<usize>(srcGlobalIdx) % sliceSize;

      usize srcSlot = 1;
      if(srcZ < destZ)
      {
        srcSlot = 0;
      }
      else if(srcZ > destZ)
      {
        srcSlot = 2;
      }

      ensureSrcLoaded(srcSlot, srcZ);

      for(usize c = 0; c < numComp; c++)
      {
        SliceBufferedTransferFunctor::bufPtr(destBuf)[inSlice * numComp + c] = SliceBufferedTransferFunctor::bufPtr(srcBufs[srcSlot])[srcInSlice * numComp + c];
      }
      modified = true;
    }

    if(modified)
    {
      store.copyFromBuffer(destZ * sliceValues, nonstd::span<const T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));
    }
  }
};

/**
 * @brief Convenience function to transfer a single Z-slice using per-slice marks.
 */
inline void SliceBufferedTransferOneZ(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
{
  ExecuteDataFunction(SliceTransferOneZFunctor{}, dataArray.getDataType(), dataArray, sliceMarks, sliceSize, destZ, dimZ);
}

} // namespace nx::core

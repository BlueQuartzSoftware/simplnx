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
 * @struct SliceBufferedTransferFunctor
 * @brief Copies neighbor tuples with a three-slice rolling source window.
 *
 * Face-neighbor morphology copies only from Z-1, Z, or Z+1. Three source
 * buffers and one destination buffer replace per-element store access with
 * sequential slice reads and conditional slice writes.
 *
 * Buffer memory is four XY slices for each array component. A single-component
 * 1024 by 1024 float32 array uses approximately 16 MiB. bool uses a raw array
 * because std::vector<bool> cannot supply a contiguous bool span.
 *
 * The transfer buffers are bounded, but the caller-owned neighbors vector has
 * one entry per volume cell. This helper is mutable and not thread-safe.
 * The rolling window preserves source values from before their destination slices change.
 */
struct SliceBufferedTransferFunctor
{
  /**
   * @brief Selects a contiguous buffer type, including bool.
   * @tparam T Specifies the array value type.
   */
  template <typename T>
  using BufferType = std::conditional_t<std::is_same_v<T, bool>, std::unique_ptr<T[]>, std::vector<T>>;

  /**
   * @brief Gets mutable vector storage.
   * @tparam T Specifies the element type.
   * @param v Supplies the vector.
   * @return Pointer to the first element.
   */
  template <typename T>
  static T* bufPtr(std::vector<T>& v)
  {
    return v.data();
  }

  /**
   * @brief Gets mutable raw-array storage.
   * @tparam T Specifies the element type.
   * @param p Supplies the owned array.
   * @return Pointer to the first element.
   */
  template <typename T>
  static T* bufPtr(std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

  /**
   * @brief Gets immutable vector storage.
   * @tparam T Specifies the element type.
   * @param v Supplies the vector.
   * @return Pointer to the first element.
   */
  template <typename T>
  static const T* bufPtr(const std::vector<T>& v)
  {
    return v.data();
  }

  /**
   * @brief Gets immutable raw-array storage.
   * @tparam T Specifies the element type.
   * @param p Supplies the owned array.
   * @return Pointer to the first element.
   */
  template <typename T>
  static const T* bufPtr(const std::unique_ptr<T[]>& p)
  {
    return p.get();
  }

  /**
   * @brief Allocates a contiguous typed buffer.
   * @tparam T Specifies the element type.
   * @param size Specifies the element count.
   * @return Buffer with size elements.
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
   * @brief Transfers all selected tuples through rolling slice buffers.
   * @tparam T Specifies the dispatched array value type.
   * @param dataArray Supplies the array to modify in place.
   * @param neighbors Maps each destination cell to a global source index, or -1.
   * @param sliceSize Specifies cells in one XY slice.
   * @param dimZ Specifies the Z-slice count.
   * @param shouldCopy Selects mapped destinations to overwrite.
   * @pre sliceSize and dimZ are nonzero. Their product equals the array tuple count.
   * @pre neighbors contains at least sliceSize times dimZ entries.
   * @pre Each nonnegative source is in range and is at most one Z slice from its destination.
   * @pre shouldCopy contains a callable target and does not throw.
   * @pre Component and slice-size products fit usize. All bulk store operations succeed.
   *
   * The function discards bulk-I/O Result values and cannot report a storage failure.
   * Modified slices are written once. Unmodified slices remain unchanged.
   */
  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
  {
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    const usize sliceValues = sliceSize * numComp;

    // Slots zero, one, and two contain Z-1, Z, and Z+1. Reuse them for all slices.
    std::array<BufferType<T>, 3> srcSlices;
    for(auto& s : srcSlices)
    {
      s = makeBuf<T>(sliceValues);
    }
    auto destSlice = makeBuf<T>(sliceValues);

    auto readSlice = [&](usize z, usize slot) { store.copyIntoBuffer(z * sliceValues, nonstd::span<T>(bufPtr(srcSlices[slot]), sliceValues)); };

    // Prime the current and next slots.
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

      // Preserve tuples that have no selected source.
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

      // Avoid a disk-backed write when the destination slice did not change.
      if(modified)
      {
        store.copyFromBuffer(zIdx * sliceValues, nonstd::span<const T>(bufPtr(destSlice), sliceValues));
      }
    }
  }
};

/**
 * @brief Dispatches a full-volume mapping to bounded transfer buffers.
 * @param dataArray Supplies the array to modify in place.
 * @param neighbors Maps each destination cell to a global source index, or -1.
 * @param sliceSize Specifies cells in one XY slice.
 * @param dimZ Specifies the Z-slice count.
 * @param shouldCopy Selects mapped destinations to overwrite.
 *
 * See SliceBufferedTransferFunctor for preconditions. The neighbor map remains
 * proportional to the complete volume even though transfer buffers are slice-bounded.
 */
inline void SliceBufferedTransfer(IDataArray& dataArray, const std::vector<int64>& neighbors, usize sliceSize, usize dimZ, const std::function<bool(usize)>& shouldCopy)
{
  ExecuteDataFunction(SliceBufferedTransferFunctor{}, dataArray.getDataType(), dataArray, neighbors, sliceSize, dimZ, shouldCopy);
}

/**
 * @struct SliceTransferOneZFunctor
 * @brief Transfers one destination slice from lazily loaded source slices.
 *
 * This variant accepts a slice-sized mark array for iterative algorithms. It
 * loads only the required Z-1, Z, and Z+1 source slices. Working memory is one
 * destination slice plus at most three source slices. The functor is not thread-safe.
 * Source values reflect the store state at the start of this one-slice call.
 */
struct SliceTransferOneZFunctor
{
  /**
   * @brief Transfers marked tuples for one Z slice.
   * @tparam T Specifies the dispatched array value type.
   * @param dataArray Supplies the array to modify in place.
   * @param sliceMarks Maps each destination position to a global source index, or -1.
   * @param sliceSize Specifies cells in one XY slice.
   * @param destZ Identifies the destination Z slice.
   * @param dimZ Specifies the total Z-slice count.
   * @pre sliceSize and dimZ are nonzero. destZ is less than dimZ.
   * @pre sliceMarks contains sliceSize entries.
   * @pre Each source is valid and is on destZ-1, destZ, or destZ+1.
   * @pre The array contains at least sliceSize times dimZ tuples.
   * @pre Component and slice-size products fit usize. All bulk store operations succeed.
   *
   * The function discards bulk-I/O Result values and cannot report a storage failure.
   */
  template <typename T>
  void operator()(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
  {
    using BufT = typename SliceBufferedTransferFunctor::BufferType<T>;
    auto& store = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize numComp = store.getNumberOfComponents();
    const usize sliceValues = sliceSize * numComp;

    // Preserve destination tuples that have no source mark.
    auto destBuf = SliceBufferedTransferFunctor::makeBuf<T>(sliceValues);
    store.copyIntoBuffer(destZ * sliceValues, nonstd::span<T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));

    // Slots zero, one, and two contain Z-1, Z, and Z+1 when loaded.
    std::array<BufT, 3> srcBufs;
    std::array<bool, 3> srcLoaded = {false, false, false};

    // Load each required source slice at most once.
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

    // Avoid a disk-backed write when the destination slice did not change.
    if(modified)
    {
      store.copyFromBuffer(destZ * sliceValues, nonstd::span<const T>(SliceBufferedTransferFunctor::bufPtr(destBuf), sliceValues));
    }
  }
};

/**
 * @brief Dispatches one slice-sized mapping to bounded transfer buffers.
 * @param dataArray Supplies the array to modify in place.
 * @param sliceMarks Maps destination positions to global source indices, or -1.
 * @param sliceSize Specifies cells in one XY slice.
 * @param destZ Identifies the destination Z slice.
 * @param dimZ Specifies the total Z-slice count.
 *
 * See SliceTransferOneZFunctor for preconditions.
 */
inline void SliceBufferedTransferOneZ(IDataArray& dataArray, const std::vector<int64>& sliceMarks, usize sliceSize, usize destZ, usize dimZ)
{
  ExecuteDataFunction(SliceTransferOneZFunctor{}, dataArray.getDataType(), dataArray, sliceMarks, sliceSize, destZ, dimZ);
}

} // namespace nx::core

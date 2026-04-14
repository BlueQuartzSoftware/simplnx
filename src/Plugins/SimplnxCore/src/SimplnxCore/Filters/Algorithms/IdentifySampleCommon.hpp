// -----------------------------------------------------------------------------
// IdentifySampleCommon.hpp -- Shared utilities for IdentifySample algorithms
// -----------------------------------------------------------------------------
//
// This header contains two components shared between the BFS and CCL variants
// of the IdentifySample algorithm:
//
// 1. VectorUnionFind: A lightweight, vector-based union-find (disjoint set)
//    data structure optimized for dense, sequentially-assigned label sets.
//    Used by the CCL variant (IdentifySampleCCL) for tracking connected
//    component equivalences during scanline labeling. Uses union-by-rank
//    and path-halving for near-O(1) amortized operations.
//
// 2. IdentifySampleSliceBySliceFunctor: A type-dispatched functor that
//    performs BFS-based sample identification on individual 2D slices of
//    the volume. Used by BOTH algorithm classes when the user enables
//    slice-by-slice mode. Since a single 2D slice always fits in memory,
//    BFS is safe and efficient regardless of whether the underlying data
//    store is in-core or out-of-core.
//
//    The functor supports three orthogonal slice planes (XY, XZ, YZ) and
//    includes a batched YZ code path that amortizes HDF5 I/O by reading
//    each Z-slice once per batch of X positions (instead of once per X
//    position), providing approximately 10x speedup for OOC data.
// -----------------------------------------------------------------------------

#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace nx::core
{

/**
 * @class VectorUnionFind
 * @brief Vector-based union-find (disjoint set) for dense, sequentially-assigned
 * label sets (labels 1..N).
 *
 * Uses flat vectors instead of hash maps for O(1) indexed access with no hash
 * overhead. Suitable for connected component labeling where labels are assigned
 * sequentially starting from 1 and the maximum label count is not known in advance
 * (internal storage grows dynamically).
 *
 * Features:
 * - Union-by-rank for balanced merges (O(alpha(N)) amortized)
 * - Path halving in find() for near-O(1) amortized lookups
 * - Dynamic growth via makeSet() -- no need to pre-size
 * - No flatten() method -- the CCL code accumulates sizes externally and
 *   resolves roots via find() after the forward scan completes
 *
 * This class is used by IdentifySampleCCL (in IdentifySampleCCL.cpp) for the
 * 3D CCL algorithm. FillBadDataCCL uses the separate UnionFind class (in
 * simplnx/Utilities/UnionFind.hpp) which includes built-in size tracking and
 * a flatten() method.
 */
class VectorUnionFind
{
public:
  VectorUnionFind() = default;

  /**
   * @brief Pre-allocates internal storage for the expected number of labels.
   * @param capacity Maximum expected label value.
   */
  void reserve(usize capacity)
  {
    m_Parent.reserve(capacity + 1);
    m_Rank.reserve(capacity + 1);
  }

  /**
   * @brief Creates a new singleton set for label x if it does not already exist.
   * @param x Label to initialize.
   */
  void makeSet(int64 x)
  {
    if(static_cast<usize>(x) >= m_Parent.size())
    {
      m_Parent.resize(x + 1, 0);
      m_Rank.resize(x + 1, 0);
    }
    if(m_Parent[x] == 0)
    {
      m_Parent[x] = x;
    }
  }

  /**
   * @brief Finds the root label with path-halving compression.
   * @param x Label to find the root for.
   * @return Root label of the equivalence class.
   */
  int64 find(int64 x)
  {
    while(m_Parent[x] != x)
    {
      m_Parent[x] = m_Parent[m_Parent[x]]; // path halving
      x = m_Parent[x];
    }
    return x;
  }

  /**
   * @brief Merges the equivalence classes of two labels using union-by-rank.
   * @param a First label.
   * @param b Second label.
   */
  void unite(int64 a, int64 b)
  {
    a = find(a);
    b = find(b);
    if(a == b)
    {
      return;
    }
    if(m_Rank[a] < m_Rank[b])
    {
      std::swap(a, b);
    }
    m_Parent[b] = a;
    if(m_Rank[a] == m_Rank[b])
    {
      m_Rank[a]++;
    }
  }

private:
  std::vector<int64> m_Parent;
  std::vector<int32> m_Rank;
};

/**
 * @struct IdentifySampleSliceBySliceFunctor
 * @brief BFS-based implementation for slice-by-slice mode.
 *
 * Slices are 2D and small relative to the full volume, so OOC chunk
 * thrashing is not a concern. This functor is used by both the in-core
 * and OOC algorithm classes when slice-by-slice mode is enabled.
 */
struct IdentifySampleSliceBySliceFunctor
{
  /**
   * @brief Enumerates the three orthogonal slice planes.
   */
  enum class Plane
  {
    XY,
    XZ,
    YZ
  };

  static constexpr int64 k_Dp1[4] = {0, 0, -1, 1};
  static constexpr int64 k_Dp2[4] = {-1, 1, 0, 0};

  /**
   * @brief BFS sample identification + optional hole filling on a single 2D slice buffer.
   *
   * Operates entirely on in-memory data. The sliceBuffer is modified in-place:
   * non-sample voxels are set to false, and if fillHoles is true, interior
   * holes (false regions not touching the boundary) are filled back to true.
   */
  template <typename T>
  static void processSlice(T* sliceBuffer, usize sliceSize, int64 planeDim1, int64 planeDim2, bool fillHoles, const std::atomic_bool& shouldCancel)
  {
    // BFS for sample identification
    std::vector<bool> checked(sliceSize, false);
    std::vector<bool> sample(sliceSize, false);
    std::vector<int64> currentVList;
    int64 biggestBlock = 0;

    for(int64 p2 = 0; p2 < planeDim2; ++p2)
    {
      for(int64 p1 = 0; p1 < planeDim1; ++p1)
      {
        int64 planeIndex = p2 * planeDim1 + p1;

        if(!checked[planeIndex] && static_cast<bool>(sliceBuffer[planeIndex]))
        {
          currentVList.push_back(planeIndex);
          int64 count = 0;

          while(count < static_cast<int64>(currentVList.size()))
          {
            int64 localIdx = currentVList[count];
            int64 localP1 = localIdx % planeDim1;
            int64 localP2 = localIdx / planeDim1;

            for(int j = 0; j < 4; ++j)
            {
              int64 neighborP1 = localP1 + k_Dp1[j];
              int64 neighborP2 = localP2 + k_Dp2[j];

              if(neighborP1 >= 0 && neighborP1 < planeDim1 && neighborP2 >= 0 && neighborP2 < planeDim2)
              {
                int64 neighborIdx = neighborP2 * planeDim1 + neighborP1;

                if(!checked[neighborIdx] && static_cast<bool>(sliceBuffer[neighborIdx]))
                {
                  currentVList.push_back(neighborIdx);
                  checked[neighborIdx] = true;
                }
              }
            }
            count++;
          }

          if(static_cast<int64>(currentVList.size()) > biggestBlock)
          {
            biggestBlock = currentVList.size();
            sample.assign(sliceSize, false);
            for(int64 idx : currentVList)
            {
              sample[idx] = true;
            }
          }
          currentVList.clear();
        }
      }
    }
    if(shouldCancel)
    {
      return;
    }

    // Mark non-sample voxels as false
    for(usize i = 0; i < sliceSize; ++i)
    {
      if(!sample[i])
      {
        sliceBuffer[i] = static_cast<T>(false);
      }
    }

    if(shouldCancel)
    {
      return;
    }

    // BFS for hole filling
    checked.assign(sliceSize, false);
    if(fillHoles)
    {
      for(int64 p2 = 0; p2 < planeDim2; ++p2)
      {
        for(int64 p1 = 0; p1 < planeDim1; ++p1)
        {
          int64 planeIndex = p2 * planeDim1 + p1;

          if(!checked[planeIndex] && !static_cast<bool>(sliceBuffer[planeIndex]))
          {
            currentVList.push_back(planeIndex);
            int64 count = 0;
            bool touchesBoundary = false;

            while(count < static_cast<int64>(currentVList.size()))
            {
              int64 localIdx = currentVList[count];
              int64 localP1 = localIdx % planeDim1;
              int64 localP2 = localIdx / planeDim1;

              if(localP1 == 0 || localP1 == planeDim1 - 1 || localP2 == 0 || localP2 == planeDim2 - 1)
              {
                touchesBoundary = true;
              }

              for(int j = 0; j < 4; ++j)
              {
                int64 neighborP1 = localP1 + k_Dp1[j];
                int64 neighborP2 = localP2 + k_Dp2[j];

                if(neighborP1 >= 0 && neighborP1 < planeDim1 && neighborP2 >= 0 && neighborP2 < planeDim2)
                {
                  int64 neighborIdx = neighborP2 * planeDim1 + neighborP1;

                  if(!checked[neighborIdx] && !static_cast<bool>(sliceBuffer[neighborIdx]))
                  {
                    currentVList.push_back(neighborIdx);
                    checked[neighborIdx] = true;
                  }
                }
              }
              count++;
            }

            if(!touchesBoundary)
            {
              for(int64 idx : currentVList)
              {
                sliceBuffer[idx] = static_cast<T>(true);
              }
            }
            currentVList.clear();
          }
        }
      }
    }
  }

  /**
   * @brief Performs BFS-based sample identification on each 2D slice of the given plane.
   * @param imageGeom The image geometry providing dimensions.
   * @param goodVoxelsPtr The mask array marking sample vs. non-sample voxels.
   * @param fillHoles Whether to fill interior holes in each slice.
   * @param plane Which orthogonal plane to slice along.
   * @param messageHandler Handler for progress messages.
   * @param shouldCancel Cancellation flag checked between slices.
   */
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, Plane plane, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    SizeVec3 uDims = imageGeom->getDimensions();
    const int64 dimX = static_cast<int64>(uDims[0]);
    const int64 dimY = static_cast<int64>(uDims[1]);
    const int64 dimZ = static_cast<int64>(uDims[2]);

    int64 planeDim1 = 0, planeDim2 = 0, fixedDim = 0;
    int64 stride1 = 0, stride2 = 0, fixedStride = 0;

    switch(plane)
    {
    case Plane::XY:
      planeDim1 = dimX;
      planeDim2 = dimY;
      fixedDim = dimZ;
      stride1 = 1;
      stride2 = dimX;
      fixedStride = dimX * dimY;
      break;

    case Plane::XZ:
      planeDim1 = dimX;
      planeDim2 = dimZ;
      fixedDim = dimY;
      stride1 = 1;
      stride2 = dimX * dimY;
      fixedStride = dimX;
      break;

    case Plane::YZ:
      planeDim1 = dimY;
      planeDim2 = dimZ;
      fixedDim = dimX;
      stride1 = dimX;
      stride2 = dimX * dimY;
      fixedStride = 1;
      break;
    }

    const usize sliceSize = static_cast<usize>(planeDim1 * planeDim2);

    // YZ batched path: read each Z-slice once per batch instead of once per X
    // position. This reduces HDF5 ops from fixedDim × dimZ to
    // ceil(fixedDim/batch) × dimZ × 3, giving ~10x speedup for OOC.
    if constexpr(!std::is_same_v<T, bool>)
    {
      if(plane == Plane::YZ)
      {
        constexpr int64 k_BatchSize = 64;
        const usize zSliceElements = static_cast<usize>(dimX * dimY);
        std::vector<T> zSliceBuf(zSliceElements);

        for(int64 batchStart = 0; batchStart < fixedDim; batchStart += k_BatchSize)
        {
          if(shouldCancel)
          {
            return;
          }

          const int64 batchEnd = std::min(batchStart + k_BatchSize, fixedDim);
          const int64 batchCount = batchEnd - batchStart;

          // Allocate column buffers for this batch
          std::vector<std::unique_ptr<T[]>> columnBuffers(static_cast<usize>(batchCount));
          for(int64 b = 0; b < batchCount; b++)
          {
            columnBuffers[static_cast<usize>(b)] = std::make_unique<T[]>(sliceSize);
          }

          // Phase A: Read each Z-slice once, extract columns for all batch members
          for(int64 z = 0; z < dimZ; z++)
          {
            goodVoxels.copyIntoBuffer(static_cast<usize>(z) * zSliceElements, nonstd::span<T>(zSliceBuf.data(), zSliceElements));
            for(int64 b = 0; b < batchCount; b++)
            {
              const int64 x = batchStart + b;
              for(int64 y = 0; y < dimY; y++)
              {
                columnBuffers[static_cast<usize>(b)][static_cast<usize>(z * dimY + y)] = zSliceBuf[static_cast<usize>(y * dimX + x)];
              }
            }
          }

          // Phase B: BFS each column buffer independently
          for(int64 b = 0; b < batchCount; b++)
          {
            if(shouldCancel)
            {
              return;
            }
            messageHandler(IFilter::Message::Type::Info, fmt::format("Slice {}", batchStart + b));
            processSlice(columnBuffers[static_cast<usize>(b)].get(), sliceSize, planeDim1, planeDim2, fillHoles, shouldCancel);
          }

          // Phase C: Write back — read each Z-slice, insert columns, write
          for(int64 z = 0; z < dimZ; z++)
          {
            goodVoxels.copyIntoBuffer(static_cast<usize>(z) * zSliceElements, nonstd::span<T>(zSliceBuf.data(), zSliceElements));
            for(int64 b = 0; b < batchCount; b++)
            {
              const int64 x = batchStart + b;
              for(int64 y = 0; y < dimY; y++)
              {
                zSliceBuf[static_cast<usize>(y * dimX + x)] = columnBuffers[static_cast<usize>(b)][static_cast<usize>(z * dimY + y)];
              }
            }
            goodVoxels.copyFromBuffer(static_cast<usize>(z) * zSliceElements, nonstd::span<const T>(zSliceBuf.data(), zSliceElements));
          }
        }
        return; // YZ batched processing complete
      }
    }

    // XY / XZ / YZ-bool path: process one plane at a time
    auto sliceBuffer = std::make_unique<T[]>(sliceSize);

    for(int64 fixedIdx = 0; fixedIdx < fixedDim; ++fixedIdx)
    {
      if(shouldCancel)
      {
        return;
      }
      messageHandler(IFilter::Message::Type::Info, fmt::format("Slice {}", fixedIdx));

      // Read the 2D slice into a local buffer using bulk reads where possible.
      if(stride1 == 1 && stride2 == planeDim1)
      {
        // XY plane: entire slice is contiguous in memory. Single bulk read.
        goodVoxels.copyIntoBuffer(static_cast<usize>(fixedIdx * fixedStride), nonstd::span<T>(sliceBuffer.get(), sliceSize));
      }
      else if(stride1 == 1)
      {
        // XZ plane: each row of planeDim1 elements is contiguous, but rows
        // are separated by stride2 (dimX*dimY). Read row-by-row.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          usize rowStart = static_cast<usize>(fixedIdx * fixedStride + p2 * stride2);
          goodVoxels.copyIntoBuffer(rowStart, nonstd::span<T>(sliceBuffer.get() + p2 * planeDim1, static_cast<usize>(planeDim1)));
        }
      }
      else
      {
        // YZ plane (bool type fallback): per-element access.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          for(int64 p1 = 0; p1 < planeDim1; ++p1)
          {
            sliceBuffer[static_cast<usize>(p2 * planeDim1 + p1)] = goodVoxels.getValue(static_cast<usize>(fixedIdx * fixedStride + p2 * stride2 + p1 * stride1));
          }
        }
      }

      processSlice(sliceBuffer.get(), sliceSize, planeDim1, planeDim2, fillHoles, shouldCancel);

      // Write the modified slice back to the DataStore using bulk writes where possible.
      if(stride1 == 1 && stride2 == planeDim1)
      {
        // XY plane: entire slice is contiguous in memory. Single bulk write.
        goodVoxels.copyFromBuffer(static_cast<usize>(fixedIdx * fixedStride), nonstd::span<const T>(sliceBuffer.get(), sliceSize));
      }
      else if(stride1 == 1)
      {
        // XZ plane: each row of planeDim1 elements is contiguous. Write row-by-row.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          usize rowStart = static_cast<usize>(fixedIdx * fixedStride + p2 * stride2);
          goodVoxels.copyFromBuffer(rowStart, nonstd::span<const T>(sliceBuffer.get() + p2 * planeDim1, static_cast<usize>(planeDim1)));
        }
      }
      else
      {
        // YZ plane (bool type fallback): per-element write-back.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          for(int64 p1 = 0; p1 < planeDim1; ++p1)
          {
            goodVoxels.setValue(static_cast<usize>(fixedIdx * fixedStride + p2 * stride2 + p1 * stride1), sliceBuffer[static_cast<usize>(p2 * planeDim1 + p1)]);
          }
        }
      }
    }
  }
};

} // namespace nx::core

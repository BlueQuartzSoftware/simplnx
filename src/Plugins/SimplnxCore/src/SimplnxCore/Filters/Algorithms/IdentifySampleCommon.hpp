#pragma once

#include <array>

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
 * @brief Stores dense label equivalences in resident vectors.
 *
 * The helper uses union by rank and path halving for labels 1 through N. It is
 * retained for resident algorithms that need dynamically growing label sets.
 * The current IdentifySample CCL path uses ExternalEquivalence instead because
 * its label records can reside on disk.
 */
class VectorUnionFind
{
public:
  VectorUnionFind() = default;

  /**
   * @brief Reserves storage through the largest expected label.
   * @param capacity Largest expected label.
   */
  void reserve(usize capacity)
  {
    m_Parent.reserve(capacity + 1);
    m_Rank.reserve(capacity + 1);
  }

  /**
   * @brief Creates a singleton set when the label is new.
   * @param x Label to initialize.
   * @pre x is positive.
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
   * @brief Finds a root and applies path halving.
   * @param x Initialized label.
   * @return Root label.
   * @pre makeSet() initialized x.
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
   * @brief Merges two sets by rank.
   * @param a Initialized label.
   * @param b Initialized label.
   * @pre makeSet() initialized a and b.
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
 * @brief Uses buffered BFS for resident slice-by-slice execution.
 *
 * This functor is the BFS path's slice implementation. The CCL path uses its own
 * row-streaming slice implementation. XY and XZ use bulk transfers. Bool YZ uses
 * individual values. Other YZ types batch as many as 64 plane buffers with one
 * Z-slice buffer to reduce repeated disk reads.
 *
 * Bulk-I/O results are discarded. Cancellation can leave prior slices changed.
 */
struct IdentifySampleSliceBySliceFunctor
{
  /**
   * @enum Plane
   * @brief Selects the orthogonal plane to process independently.
   */
  enum class Plane
  {
    XY,
    XZ,
    YZ
  };

  static constexpr std::array<int64, 4> k_Dp1 = {0, 0, -1, 1};
  static constexpr std::array<int64, 4> k_Dp2 = {-1, 1, 0, 0};

  /**
   * @brief Retains the largest true plane component and optionally fills holes.
   * @tparam T Mask value type.
   * @param sliceBuffer Plane values modified in place.
   * @param sliceSize Number of plane values.
   * @param planeDim1 Fast plane dimension.
   * @param planeDim2 Slow plane dimension.
   * @param fillHoles True to fill false components that do not touch an edge.
   * @param shouldCancel Signals cancellation after component discovery or removal.
   * @pre sliceBuffer contains at least sliceSize values.
   * @pre sliceSize equals planeDim1 times planeDim2.
   *
   * Two N-bit vectors and a component queue retain plane state. The initial seed
   * is not marked when it enters the queue, so a back edge can enqueue it twice.
   * Equal-sized components favor the component that the scan finds last.
   * Cancellation is not checked within a component search.
   */
  template <typename T>
  static void processSlice(T* sliceBuffer, usize sliceSize, int64 planeDim1, int64 planeDim2, bool fillHoles, const std::atomic_bool& shouldCancel)
  {
    // Find the largest true plane component.
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

          if(static_cast<int64>(currentVList.size()) >= biggestBlock)
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

    // Remove true values outside the selected component.
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

    // Fill false components that do not touch a plane edge.
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
   * @brief Processes each selected plane through a buffered BFS.
   * @tparam T Mask value type.
   * @param imageGeom Supplies dimensions.
   * @param goodVoxelsPtr Mask modified in place.
   * @param fillHoles True to fill false components that do not touch a plane edge.
   * @param plane Selected plane orientation.
   * @param messageHandler Receives slice messages.
   * @param shouldCancel Signals cancellation between planes and BFS phases.
   * @pre imageGeom and goodVoxelsPtr are not null.
   *
   * The non-Bool YZ path can retain 64 YZ planes and one XY Z-slice at the same
   * time. Other paths retain one selected plane. The function ignores all
   * copyIntoBuffer() and copyFromBuffer() results.
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

    // Batch YZ columns so that one Z-slice read supplies as many as 64 planes.
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

          // Retain one plane buffer for each X position in this batch.
          std::vector<std::unique_ptr<T[]>> columnBuffers(static_cast<usize>(batchCount));
          for(int64 b = 0; b < batchCount; b++)
          {
            columnBuffers[static_cast<usize>(b)] = std::make_unique<T[]>(sliceSize);
          }

          // Read each Z-slice once and extract all batch columns.
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

          // Process each column plane independently.
          for(int64 b = 0; b < batchCount; b++)
          {
            if(shouldCancel)
            {
              return;
            }
            processSlice(columnBuffers[static_cast<usize>(b)].get(), sliceSize, planeDim1, planeDim2, fillHoles, shouldCancel);
          }

          // Read each Z-slice, insert all batch columns, and write it back.
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
        return;
      }
    }

    // Process one XY, XZ, or Bool YZ plane at a time.
    auto sliceBuffer = std::make_unique<T[]>(sliceSize);

    for(int64 fixedIdx = 0; fixedIdx < fixedDim; ++fixedIdx)
    {
      if(shouldCancel)
      {
        return;
      }
      messageHandler(IFilter::Message::Type::Info, fmt::format("Slice {}", fixedIdx));

      // Read the plane with bulk transfers where its layout permits them.
      if(stride1 == 1 && stride2 == planeDim1)
      {
        // An XY plane is contiguous.
        goodVoxels.copyIntoBuffer(static_cast<usize>(fixedIdx * fixedStride), nonstd::span<T>(sliceBuffer.get(), sliceSize));
      }
      else if(stride1 == 1)
      {
        // Each XZ row is contiguous.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          usize rowStart = static_cast<usize>(fixedIdx * fixedStride + p2 * stride2);
          goodVoxels.copyIntoBuffer(rowStart, nonstd::span<T>(sliceBuffer.get() + p2 * planeDim1, static_cast<usize>(planeDim1)));
        }
      }
      else
      {
        // AbstractDataStore<bool> requires individual YZ values here.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          for(int64 p1 = 0; p1 < planeDim1; ++p1)
          {
            sliceBuffer[static_cast<usize>(p2 * planeDim1 + p1)] = goodVoxels.getValue(static_cast<usize>(fixedIdx * fixedStride + p2 * stride2 + p1 * stride1));
          }
        }
      }

      processSlice(sliceBuffer.get(), sliceSize, planeDim1, planeDim2, fillHoles, shouldCancel);

      // Write the plane with bulk transfers where its layout permits them.
      if(stride1 == 1 && stride2 == planeDim1)
      {
        // An XY plane is contiguous.
        goodVoxels.copyFromBuffer(static_cast<usize>(fixedIdx * fixedStride), nonstd::span<const T>(sliceBuffer.get(), sliceSize));
      }
      else if(stride1 == 1)
      {
        // Each XZ row is contiguous.
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          usize rowStart = static_cast<usize>(fixedIdx * fixedStride + p2 * stride2);
          goodVoxels.copyFromBuffer(rowStart, nonstd::span<const T>(sliceBuffer.get() + p2 * planeDim1, static_cast<usize>(planeDim1)));
        }
      }
      else
      {
        // AbstractDataStore<bool> requires individual YZ values here.
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

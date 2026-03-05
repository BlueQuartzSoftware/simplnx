#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <atomic>
#include <vector>

namespace nx::core
{

/**
 * @class VectorUnionFind
 * @brief Vector-based union-find for dense label sets (labels 1..N).
 *
 * Uses flat vectors instead of hash maps for O(1) access. Suitable for
 * connected component labeling where labels are assigned sequentially.
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

    int64 planeDim1, planeDim2, fixedDim;
    int64 stride1, stride2, fixedStride;

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

    for(int64 fixedIdx = 0; fixedIdx < fixedDim; ++fixedIdx)
    {
      if(shouldCancel)
      {
        return;
      }
      messageHandler(IFilter::Message::Type::Info, fmt::format("Slice {}", fixedIdx));

      std::vector<bool> checked(planeDim1 * planeDim2, false);
      std::vector<bool> sample(planeDim1 * planeDim2, false);
      std::vector<int64> currentVList;
      int64 biggestBlock = 0;

      for(int64 p2 = 0; p2 < planeDim2; ++p2)
      {
        for(int64 p1 = 0; p1 < planeDim1; ++p1)
        {
          int64 planeIndex = p2 * planeDim1 + p1;
          int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

          if(!checked[planeIndex] && goodVoxels.getValue(globalIndex))
          {
            currentVList.push_back(planeIndex);
            int64 count = 0;

            while(count < currentVList.size())
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
                  int64 globalNeighborIdx = fixedIdx * fixedStride + neighborP2 * stride2 + neighborP1 * stride1;

                  if(!checked[neighborIdx] && goodVoxels.getValue(globalNeighborIdx))
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
              sample.assign(planeDim1 * planeDim2, false);
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

      for(int64 p2 = 0; p2 < planeDim2; ++p2)
      {
        for(int64 p1 = 0; p1 < planeDim1; ++p1)
        {
          int64 planeIndex = p2 * planeDim1 + p1;
          int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

          if(!sample[planeIndex])
          {
            goodVoxels.setValue(globalIndex, false);
          }
        }
      }
      if(shouldCancel)
      {
        return;
      }
      if(fillHoles)
      {
        for(int64 p2 = 0; p2 < planeDim2; ++p2)
        {
          for(int64 p1 = 0; p1 < planeDim1; ++p1)
          {
            int64 planeIndex = p2 * planeDim1 + p1;
            int64 globalIndex = fixedIdx * fixedStride + p2 * stride2 + p1 * stride1;

            if(!checked[planeIndex] && !goodVoxels.getValue(globalIndex))
            {
              currentVList.push_back(planeIndex);
              int64 count = 0;
              bool touchesBoundary = false;

              while(count < currentVList.size())
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
                    int64 globalNeighborIdx = fixedIdx * fixedStride + neighborP2 * stride2 + neighborP1 * stride1;

                    if(!checked[neighborIdx] && !goodVoxels.getValue(globalNeighborIdx))
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
                  int64 fillP1 = idx % planeDim1;
                  int64 fillP2 = idx / planeDim1;
                  goodVoxels.setValue(fixedIdx * fixedStride + fillP2 * stride2 + fillP1 * stride1, true);
                }
              }
              currentVList.clear();
            }
          }
        }
      }
    }
  }
};

} // namespace nx::core

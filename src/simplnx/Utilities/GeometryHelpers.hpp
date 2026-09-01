#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/EulerAngle.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include <Eigen/Dense>

#include <memory>
#include <set>
#include <unordered_set>

/**
 * @namespace nx::core::GeometryHelpers
 * @brief Provides geometry description, connectivity, and topology utilities.
 */
namespace nx::core::GeometryHelpers
{
/**
 * @typedef ErrorCode
 * @brief Defines integer status values for connectivity utilities.
 */
using ErrorCode = int32;

/**
 * @namespace Description
 * @brief Provides user-readable geometry descriptions.
 */
namespace Description
{

/**
 * @brief Formats ImageGeom dimensions and spatial metadata.
 * @param dims Specifies image dimensions.
 * @param spacing Specifies axis spacing.
 * @param origin Specifies image origin.
 * @param units Specifies length units.
 * @return Geometry description.
 */
SIMPLNX_EXPORT std::string GenerateGeometryInfo(const nx::core::SizeVec3& dims, const nx::core::FloatVec3& spacing, const nx::core::FloatVec3& origin, IGeometry::LengthUnit units);

} // namespace Description

/**
 * @namespace Connectivity
 * @brief Provides mesh adjacency and boundary extraction utilities.
 */
namespace Connectivity
{
/**
 * @namespace detail
 * @brief Provides internal edge-count implementations.
 */
namespace detail
{
inline constexpr uint64 k_MaxOptimizedValue = static_cast<uint64>(std::numeric_limits<uint32>::max());

/**
 * @brief Counts unique edges without narrowing vertex indexes.
 * @tparam T Specifies the face-index type.
 * @param faceStore Provides polygon vertex indexes.
 * @return Unique edge count.
 * @note Use FindNumEdges() for automatic implementation selection.
 */
template <typename T>
usize SafeEdgeCount(const AbstractDataStore<T>& faceStore)
{
  const usize numFaces = faceStore.getNumberOfTuples();
  const usize numComp = faceStore.getNumberOfComponents();
  T v0 = 0;
  T v1 = 0;

  std::set<std::pair<T, T>> edgeSet;

  for(usize i = 0; i < numFaces; i++)
  {
    const usize offset = i * numComp;

    for(usize j = 0; j < numComp; j++)
    {
      if(j == (numComp - 1))
      {
        if(faceStore[offset + j] > faceStore[offset + 0])
        {
          v0 = faceStore[offset + 0];
          v1 = faceStore[offset + j];
        }
        else
        {
          v0 = faceStore[offset + j];
          v1 = faceStore[offset + 0];
        }
      }
      else
      {
        if(faceStore[offset + j] > faceStore[offset + j + 1])
        {
          v0 = faceStore[offset + j + 1];
          v1 = faceStore[offset + j];
        }
        else
        {
          v0 = faceStore[offset + j];
          v1 = faceStore[offset + j + 1];
        }
      }
      std::pair<T, T> edge = std::make_pair(v0, v1);
      edgeSet.insert(edge);
    }
  }

  return edgeSet.size();
}

/**
 * @brief Counts unique edges with packed UInt32 vertex pairs.
 * @tparam T Specifies the face-index type.
 * @param faceStore Provides polygon vertex indexes that fit UInt32.
 * @return Unique edge count.
 * @note Use FindNumEdges() for range-based implementation selection.
 */
template <typename T>
usize FastEdgeCount(const AbstractDataStore<T>& faceStore)
{
  const usize numFaces = faceStore.getNumberOfTuples();
  const usize numComp = faceStore.getNumberOfComponents();
  uint32 v0 = 0;
  uint32 v1 = 0;

  std::unordered_set<uint64> edgeSet;

  for(usize i = 0; i < numFaces; i++)
  {
    const usize offset = i * numComp;

    for(usize j = 0; j < numComp; j++)
    {
      if(j == (numComp - 1))
      {
        if(faceStore[offset + j] > faceStore[offset + 0])
        {
          v0 = static_cast<uint32>(faceStore[offset + 0]);
          v1 = static_cast<uint32>(faceStore[offset + j]);
        }
        else
        {
          v0 = static_cast<uint32>(faceStore[offset + j]);
          v1 = static_cast<uint32>(faceStore[offset + 0]);
        }
      }
      else
      {
        if(faceStore[offset + j] > faceStore[offset + j + 1])
        {
          v0 = static_cast<uint32>(faceStore[offset + j + 1]);
          v1 = static_cast<uint32>(faceStore[offset + j]);
        }
        else
        {
          v0 = static_cast<uint32>(faceStore[offset + j]);
          v1 = static_cast<uint32>(faceStore[offset + j + 1]);
        }
      }

      edgeSet.insert(static_cast<uint64>(v0) << 32 | v1);
    }
  }

  return edgeSet.size();
}
} // namespace detail

/**
 * @brief Computes one Euler characteristic for each labeled mesh region.
 * @param triangleGeom Provides triangle connectivity.
 * @param faceLabelsRef Provides two region labels per face.
 * @return Euler characteristic values indexed by region ID.
 */
SIMPLNX_EXPORT std::vector<int32> FindEulerCharacteristicValues(const TriangleGeom& triangleGeom, const Int32Array& faceLabelsRef);

/**
 * @brief Selects a unique-edge count implementation from vertex-index range.
 * @tparam T Specifies the face-index type.
 * @param faceStore Provides polygon vertex indexes.
 * @param numVertices Specifies vertex count for packed-index safety.
 * @return Unique edge count.
 *
 * The packed implementation is faster but requires unsigned indexes below UInt32 maximum.
 */
template <typename T>
usize FindNumEdges(const AbstractDataStore<T>& faceStore, usize numVertices = (detail::k_MaxOptimizedValue + 1))
{
  if constexpr(!std::is_signed_v<T>)
  {
    if(numVertices < detail::k_MaxOptimizedValue)
    {
      return detail::FastEdgeCount(faceStore);
    }
  }
  return detail::SafeEdgeCount(faceStore);
}

/**
 * @brief Builds the element list for each vertex through two chunked passes.
 * @tparam T Specifies per-vertex list-size type.
 * @tparam K Specifies mesh-index type.
 * @param elemList Provides element vertex indexes.
 * @param dynamicList Receives element indexes for each vertex.
 * @param numVerts Specifies vertex count.
 *
 * The method does not inspect source bulk-read results.
 */
template <typename T, typename K>
void FindElementsContainingVert(const DataArray<K>* elemList, DynamicListArray<T, K>* dynamicList, usize numVerts)
{
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
  const auto& elemStore = elemList->getDataStoreRef();

  std::vector<T> linkCount(numVerts, 0);
  std::vector<K> linkLoc(numVerts, static_cast<K>(0));

  // Chunked source reads avoid per-index access to disk-backed connectivity.
  constexpr usize k_ChunkElems = 65536;
  auto chunkBuf = std::make_unique<K[]>(k_ChunkElems * numVertsPerElem);

  // The first pass counts references for exact list allocation.
  for(usize start = 0; start < numElems; start += k_ChunkElems)
  {
    usize count = std::min(k_ChunkElems, numElems - start);
    usize elemCount = count * numVertsPerElem;
    elemStore.copyIntoBuffer(start * numVertsPerElem, nonstd::span<K>(chunkBuf.get(), elemCount));
    for(usize i = 0; i < count; i++)
    {
      for(usize j = 0; j < numVertsPerElem; j++)
      {
        ++linkCount[chunkBuf[i * numVertsPerElem + j]];
      }
    }
  }

  dynamicList->allocateLists(linkCount);

  // The second pass writes element references.
  for(usize start = 0; start < numElems; start += k_ChunkElems)
  {
    usize count = std::min(k_ChunkElems, numElems - start);
    usize elemCount = count * numVertsPerElem;
    elemStore.copyIntoBuffer(start * numVertsPerElem, nonstd::span<K>(chunkBuf.get(), elemCount));
    for(usize i = 0; i < count; i++)
    {
      usize elemId = start + i;
      for(usize j = 0; j < numVertsPerElem; j++)
      {
        K vertId = chunkBuf[i * numVertsPerElem + j];
        dynamicList->insertCellReference(vertId, (linkLoc[vertId])++, elemId);
      }
    }
  }
}

/**
 * @brief Finds neighboring elements that share a complete boundary entity.
 * @tparam T Specifies dynamic-list size type.
 * @tparam K Specifies mesh-index type.
 * @param elemList Provides element vertex indexes.
 * @param elemsContainingVert Provides candidate elements for each vertex.
 * @param dynamicList Receives neighbors for each element.
 * @param geometryType Selects required shared-vertex count.
 * @return -1 for an unsupported geometry type, or 0 after processing.
 *
 * Outer connectivity reads use chunks. Candidate reads outside the active chunk
 * use one bulk read. Source read results are not inspected.
 */
template <typename T, typename K>
ErrorCode FindElementNeighbors(const DataArray<K>* elemList, const DynamicListArray<T, K>* elemsContainingVert, DynamicListArray<T, K>* dynamicList, IGeometry::Type geometryType)
{
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
  const auto& elemStore = elemList->getDataStoreRef();
  usize numSharedVerts = 0;
  std::vector<T> linkCount(numElems, 0);
  ErrorCode err = 0;

  switch(geometryType)
  {
  case IGeometry::Type::Edge: // edges
  {
    numSharedVerts = 1;
    break;
  }
  case IGeometry::Type::Triangle: // triangles
  {
    numSharedVerts = 2;
    break;
  }
  case IGeometry::Type::Quad: // quadrilaterals
  {
    numSharedVerts = 2;
    break;
  }
  case IGeometry::Type::Tetrahedral: // tetrahedra
  {
    numSharedVerts = 3;
    break;
  }
  case IGeometry::Type::Hexahedral: // hexahedra
  {
    numSharedVerts = 4;
    break;
  }
  default:
    numSharedVerts = 0;
    break;
  }

  if(numSharedVerts == 0)
  {
    return -1;
  }

  dynamicList->allocateLists(linkCount);

  // Track candidates already accepted for the current source element.
  std::vector<uint8> visited(numElems, 0);

  // Reuse one neighbor vector across source elements.
  std::vector<K> loop_neighbors(32, 0);

  // Keep one candidate element outside the active chunk in local memory.
  auto neighborVertsBuf = std::make_unique<K[]>(numVertsPerElem);

  // Process source elements in sequential connectivity chunks.
  constexpr usize k_ChunkElems = 65536;
  auto chunkBuf = std::make_unique<K[]>(k_ChunkElems * numVertsPerElem);

  for(usize chunkStart = 0; chunkStart < numElems; chunkStart += k_ChunkElems)
  {
    usize chunkCount = std::min(k_ChunkElems, numElems - chunkStart);
    elemStore.copyIntoBuffer(chunkStart * numVertsPerElem, nonstd::span<K>(chunkBuf.get(), chunkCount * numVertsPerElem));

    for(usize ci = 0; ci < chunkCount; ci++)
    {
      usize t = chunkStart + ci;
      usize localOffset = ci * numVertsPerElem;

      for(usize v = 0; v < numVertsPerElem; v++)
      {
        K vertId = chunkBuf[localOffset + v];
        T nEs = elemsContainingVert->getNumberOfElements(vertId);
        K* vertIdxs = elemsContainingVert->getElementListPointer(vertId);

        for(T vt = 0; vt < nEs; vt++)
        {
          if(vertIdxs[vt] == static_cast<K>(t))
          {
            continue; // This is the same element as our "source"
          }
          if(visited[vertIdxs[vt]])
          {
            continue; // We already added this element so loop again
          }

          // Reuse the active chunk or read one exterior candidate.
          K candidateElem = vertIdxs[vt];
          const K* candidateVerts = nullptr;
          if(candidateElem >= chunkStart && candidateElem < chunkStart + chunkCount)
          {
            candidateVerts = &chunkBuf[(candidateElem - chunkStart) * numVertsPerElem];
          }
          else
          {
            elemStore.copyIntoBuffer(candidateElem * numVertsPerElem, nonstd::span<K>(neighborVertsBuf.get(), numVertsPerElem));
            candidateVerts = neighborVertsBuf.get();
          }

          // Count shared vertices between this source and candidate.
          usize vCount = 0;
          for(usize i = 0; i < numVertsPerElem; i++)
          {
            for(usize j = 0; j < numVertsPerElem; j++)
            {
              if(chunkBuf[localOffset + i] == candidateVerts[j])
              {
                vCount++;
              }
            }
          }

          // A complete shared boundary entity defines one neighbor.
          if(vCount == numSharedVerts)
          {
            loop_neighbors[linkCount[t]] = vertIdxs[vt];
            ++linkCount[t];
            if(linkCount[t] >= loop_neighbors.size())
            {
              loop_neighbors.resize(loop_neighbors.size() + 10);
            }
            visited[vertIdxs[vt]] = true;
          }
        }
      }

      // Clear only visited indexes recorded for this source element.
      for(int64 k = 0; k < linkCount[t]; k++)
      {
        visited[loop_neighbors[k]] = false;
      }
      dynamicList->setElementList(t, linkCount[t], &(loop_neighbors[0]));
    }
  }

  return err;
}

/**
 * @brief Extracts all unique tetrahedron edges.
 * @tparam T Specifies mesh-index type.
 * @param tetList Provides four vertex indexes per tetrahedron.
 * @param edgeList Receives sorted two-index edges.
 */
template <typename T>
void FindTetEdges(const DataArray<T>* tetList, DataArray<T>* edgeList)
{
  const usize numElems = tetList->getNumberOfTuples();
  const usize numVertsPerTet = tetList->getNumberOfComponents();
  auto& tets = *tetList;

  std::set<std::pair<T, T>> edgeSet;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerTet;

    std::vector<T> edge0 = {tets[offset + 0], tets[offset + 1]};
    std::vector<T> edge1 = {tets[offset + 0], tets[offset + 2]};
    std::vector<T> edge2 = {tets[offset + 1], tets[offset + 2]};
    std::vector<T> edge3 = {tets[offset + 0], tets[offset + 3]};
    std::vector<T> edge4 = {tets[offset + 1], tets[offset + 3]};
    std::vector<T> edge5 = {tets[offset + 2], tets[offset + 3]};
    std::list<std::vector<T>> edgeVecList = {edge0, edge1, edge2, edge3, edge4, edge5};

    for(auto&& uEdge : edgeVecList)
    {
      std::sort(uEdge.begin(), uEdge.end());
      std::pair<T, T> edge = std::make_pair(uEdge[0], uEdge[1]);
      edgeSet.insert(edge);
    }
  }

  edgeList->getDataStore()->resizeTuples({edgeSet.size()});
  auto& uEdges = *edgeList;
  T index = 0;

  for(auto& edge : edgeSet)
  {
    uEdges[2 * index] = edge.first;
    uEdges[2 * index + 1] = edge.second;
    ++index;
  }
}

/**
 * @brief Extracts all unique hexahedron edges.
 * @tparam T Specifies mesh-index type.
 * @param hexList Provides eight vertex indexes per hexahedron.
 * @param edge_List Receives sorted two-index edges.
 */
template <typename T>
void FindHexEdges(const DataArray<T>* hexList, DataArray<T>* edge_List)
{
  const usize numElems = hexList->getNumberOfTuples();
  const usize numVertsPerHex = hexList->getNumberOfComponents();

  auto& hexas = *hexList;

  std::set<std::pair<T, T>> edgeSet;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerHex;

    std::vector<T> edge0 = {hexas[offset + 0], hexas[offset + 1]};
    std::vector<T> edge1 = {hexas[offset + 1], hexas[offset + 2]};
    std::vector<T> edge2 = {hexas[offset + 2], hexas[offset + 3]};
    std::vector<T> edge3 = {hexas[offset + 3], hexas[offset + 0]};

    std::vector<T> edge4 = {hexas[offset + 0], hexas[offset + 4]};
    std::vector<T> edge5 = {hexas[offset + 1], hexas[offset + 5]};
    std::vector<T> edge6 = {hexas[offset + 2], hexas[offset + 6]};
    std::vector<T> edge7 = {hexas[offset + 3], hexas[offset + 7]};

    std::vector<T> edge8 = {hexas[offset + 4], hexas[offset + 5]};
    std::vector<T> edge9 = {hexas[offset + 5], hexas[offset + 6]};
    std::vector<T> edge10 = {hexas[offset + 6], hexas[offset + 7]};
    std::vector<T> edge11 = {hexas[offset + 7], hexas[offset + 4]};

    std::list<std::vector<T>> edgeList = {edge0, edge1, edge2, edge3, edge4, edge5, edge6, edge7, edge8, edge9, edge10, edge11};

    for(auto&& uEdge : edgeList)
    {
      std::sort(uEdge.begin(), uEdge.end());
      std::pair<T, T> edge = std::make_pair(uEdge[0], uEdge[1]);
      edgeSet.insert(edge);
    }
  }

  typename std::set<std::pair<T, T>>::iterator setIter;
  edge_List->getDataStore()->resizeTuples({edgeSet.size()});
  auto& uEdges = *edge_List;
  T index = 0;

  for(auto edge : edgeSet)
  {
    uEdges[2 * index] = edge.first;
    uEdges[2 * index + 1] = edge.second;
    ++index;
  }
}

/**
 * @brief Extracts all unique tetrahedron faces.
 * @tparam T Specifies mesh-index type.
 * @param tetList Provides four vertex indexes per tetrahedron.
 * @param faceList Receives sorted three-index faces.
 */
template <typename T>
void FindTetFaces(const DataArray<T>* tetList, DataArray<T>* faceList)
{
  auto& tets = *tetList;
  const usize numElems = tetList->getNumberOfTuples();
  const usize numVertsPerTet = tetList->getNumberOfComponents();

  std::set<std::tuple<T, T, T>> faceSet;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerTet;

    std::vector<T> tri0 = {tets[offset + 0], tets[offset + 1], tets[offset + 2]};
    std::vector<T> tri1 = {tets[offset + 1], tets[offset + 2], tets[offset + 3]};
    std::vector<T> tri2 = {tets[offset + 0], tets[offset + 2], tets[offset + 3]};
    std::vector<T> tri3 = {tets[offset + 0], tets[offset + 1], tets[offset + 3]};
    std::list<std::vector<T>> triList = {tri0, tri1, tri2, tri3};

    for(auto&& tri : triList)
    {
      std::sort(tri.begin(), tri.end());
      std::tuple<T, T, T> face = std::make_tuple(tri[0], tri[1], tri[2]);
      faceSet.insert(face);
    }
  }

  faceList->getDataStore()->resizeTuples({faceSet.size()});
  auto& uFaces = *faceList;
  T index = 0;

  for(auto& face : faceSet)
  {
    uFaces[3 * index] = std::get<0>(face);
    uFaces[3 * index + 1] = std::get<1>(face);
    uFaces[3 * index + 2] = std::get<2>(face);
    ++index;
  }
}

/**
 * @brief Extracts all unique hexahedron faces.
 * @tparam T Specifies mesh-index type.
 * @param hexList Provides eight vertex indexes per hexahedron.
 * @param faceList Receives sorted four-index faces.
 */
template <typename T>
void FindHexFaces(const DataArray<T>* hexList, DataArray<T>* faceList)
{
  auto& hexas = *hexList;
  const usize numElems = hexList->getNumberOfTuples();
  const usize numVertsPerHex = hexList->getNumberOfComponents();

  std::set<std::tuple<T, T, T, T>> faceSet;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerHex;

    std::vector<T> quad0 = {hexas[offset + 0], hexas[offset + 1], hexas[offset + 5], hexas[offset + 4]};
    std::vector<T> quad1 = {hexas[offset + 1], hexas[offset + 2], hexas[offset + 6], hexas[offset + 5]};
    std::vector<T> quad2 = {hexas[offset + 2], hexas[offset + 3], hexas[offset + 7], hexas[offset + 6]};
    std::vector<T> quad3 = {hexas[offset + 3], hexas[offset + 0], hexas[offset + 4], hexas[offset + 7]};
    std::vector<T> quad4 = {hexas[offset + 0], hexas[offset + 1], hexas[offset + 2], hexas[offset + 3]};
    std::vector<T> quad5 = {hexas[offset + 4], hexas[offset + 5], hexas[offset + 6], hexas[offset + 7]};

    std::list<std::vector<T>> quadList = {quad0, quad1, quad2, quad3, quad4, quad5};

    for(auto&& quad : quadList)
    {
      std::sort(quad.begin(), quad.end());
      std::tuple<T, T, T, T> face = std::make_tuple(quad[0], quad[1], quad[2], quad[3]);
      faceSet.insert(face);
    }
  }

  faceList->getDataStore()->resizeTuples({faceSet.size()});
  auto& uFaces = *faceList;
  T index = 0;

  for(auto& face : faceSet)
  {
    uFaces[4 * index] = std::get<0>(face);
    uFaces[4 * index + 1] = std::get<1>(face);
    uFaces[4 * index + 2] = std::get<2>(face);
    uFaces[4 * index + 3] = std::get<3>(face);
    ++index;
  }
}

/**
 * @brief Extracts tetrahedron edges referenced exactly once.
 * @tparam T Specifies mesh-index type and reference-count type.
 * @param tetList Provides four vertex indexes per tetrahedron.
 * @param edgeList Receives sorted boundary edges.
 */
template <typename T>
void FindUnsharedTetEdges(const DataArray<T>* tetList, DataArray<T>* edgeList)
{
  auto& tets = *tetList;
  const usize numElems = tetList->getNumberOfTuples();
  const usize numVertsPerTet = tetList->getNumberOfComponents();

  std::map<std::pair<T, T>, T> edgeMap;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerTet;

    std::vector<T> edge0 = {tets[offset + 0], tets[offset + 1]};
    std::vector<T> edge1 = {tets[offset + 0], tets[offset + 2]};
    std::vector<T> edge2 = {tets[offset + 1], tets[offset + 2]};
    std::vector<T> edge3 = {tets[offset + 0], tets[offset + 3]};
    std::vector<T> edge4 = {tets[offset + 1], tets[offset + 3]};
    std::vector<T> edge5 = {tets[offset + 2], tets[offset + 3]};
    std::list<std::vector<T>> edgeVecList = {edge0, edge1, edge2, edge3, edge4, edge5};

    for(auto&& uEdge : edgeVecList)
    {
      std::sort(uEdge.begin(), uEdge.end());
      std::pair<T, T> edge = std::make_pair(uEdge[0], uEdge[1]);
      ++edgeMap[edge];
    }
  }

  typename std::map<std::pair<T, T>, T>::iterator mapIter = edgeMap.begin();

  while(mapIter != edgeMap.end())
  {
    if((*mapIter).second > 1)
    {
      edgeMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }

  edgeList->getDataStore()->resizeTuples({edgeMap.size()});
  auto& bEdges = *edgeList;
  T index = 0;

  for(auto& pair : edgeMap)
  {
    bEdges[2 * index] = pair.first.first;
    bEdges[2 * index + 1] = pair.first.second;
    ++index;
  }
}

/**
 * @brief Extracts hexahedron edges referenced exactly once.
 * @tparam T Specifies mesh-index type and reference-count type.
 * @param hexList Provides eight vertex indexes per hexahedron.
 * @param edge_List Receives sorted boundary edges.
 */
template <typename T>
void FindUnsharedHexEdges(const DataArray<T>* hexList, DataArray<T>* edge_List)
{
  const usize numElems = hexList->getNumberOfTuples();
  const usize numVertsPerHex = hexList->getNumberOfComponents();
  auto& hexas = *hexList;

  std::pair<T, T> edge;
  std::map<std::pair<T, T>, T> edgeMap;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerHex;

    std::vector<T> edge0 = {hexas[offset + 0], hexas[offset + 1]};
    std::vector<T> edge1 = {hexas[offset + 1], hexas[offset + 2]};
    std::vector<T> edge2 = {hexas[offset + 2], hexas[offset + 3]};
    std::vector<T> edge3 = {hexas[offset + 3], hexas[offset + 0]};

    std::vector<T> edge4 = {hexas[offset + 0], hexas[offset + 4]};
    std::vector<T> edge5 = {hexas[offset + 1], hexas[offset + 5]};
    std::vector<T> edge6 = {hexas[offset + 2], hexas[offset + 6]};
    std::vector<T> edge7 = {hexas[offset + 3], hexas[offset + 7]};

    std::vector<T> edge8 = {hexas[offset + 4], hexas[offset + 5]};
    std::vector<T> edge9 = {hexas[offset + 5], hexas[offset + 6]};
    std::vector<T> edge10 = {hexas[offset + 6], hexas[offset + 7]};
    std::vector<T> edge11 = {hexas[offset + 7], hexas[offset + 4]};

    std::list<std::vector<T>> edgeList = {edge0, edge1, edge2, edge3, edge4, edge5, edge6, edge7, edge8, edge9, edge10, edge11};

    for(auto&& uEdge : edgeList)
    {
      std::sort(uEdge.begin(), uEdge.end());
      edge = std::make_pair(uEdge[0], uEdge[1]);
      ++edgeMap[edge];
    }
  }

  typename std::map<std::pair<T, T>, T>::iterator mapIter = edgeMap.begin();

  while(mapIter != edgeMap.end())
  {
    if((*mapIter).second > 1)
    {
      edgeMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }

  edge_List->getDataStore()->resizeTuples({edgeMap.size()});
  auto& bEdges = *edge_List;
  T index = 0;

  for(auto& pair : edgeMap)
  {
    bEdges[2 * index] = pair.first.first;
    bEdges[2 * index + 1] = pair.first.second;
    ++index;
  }
}

/**
 * @brief Extracts tetrahedron faces referenced exactly once.
 * @tparam T Specifies mesh-index type and reference-count type.
 * @param tetList Provides four vertex indexes per tetrahedron.
 * @param faceList Receives sorted boundary faces.
 */
template <typename T>
void FindUnsharedTetFaces(const DataArray<T>* tetList, DataArray<T>* faceList)
{
  const usize numElems = tetList->getNumberOfTuples();
  const usize numVertsPerTet = tetList->getNumberOfComponents();
  auto& tets = *tetList;

  std::tuple<T, T, T> face;
  std::map<std::tuple<T, T, T>, T> faceMap;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerTet;

    std::vector<T> tri0 = {tets[offset + 0], tets[offset + 1], tets[offset + 2]};
    std::vector<T> tri1 = {tets[offset + 1], tets[offset + 2], tets[offset + 3]};
    std::vector<T> tri2 = {tets[offset + 0], tets[offset + 2], tets[offset + 3]};
    std::vector<T> tri3 = {tets[offset + 0], tets[offset + 1], tets[offset + 3]};
    std::list<std::vector<T>> triList = {tri0, tri1, tri2, tri3};

    for(auto&& tri : triList)
    {
      std::sort(tri.begin(), tri.end());
      face = std::make_tuple(tri[0], tri[1], tri[2]);
      ++faceMap[face];
    }
  }

  typename std::map<std::tuple<T, T, T>, T>::iterator mapIter = faceMap.begin();

  while(mapIter != faceMap.end())
  {
    if((*mapIter).second > 1)
    {
      faceMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }

  faceList->getDataStore()->resizeTuples({faceMap.size()});
  auto& uFaces = *faceList;
  T index = 0;

  for(auto& pair : faceMap)
  {
    uFaces[3 * index] = std::get<0>(pair.first);
    uFaces[3 * index + 1] = std::get<1>(pair.first);
    uFaces[3 * index + 2] = std::get<2>(pair.first);
    ++index;
  }
}

/**
 * @brief Extracts hexahedron faces referenced exactly once.
 * @tparam T Specifies mesh-index type and reference-count type.
 * @param hexList Provides eight vertex indexes per hexahedron.
 * @param faceList Receives sorted boundary faces.
 */
template <typename T>
void FindUnsharedHexFaces(const DataArray<T>* hexList, DataArray<T>* faceList)
{
  auto& hexas = *hexList;
  const usize numElems = hexList->getNumberOfTuples();
  const usize numVertsPerHex = hexList->getNumberOfComponents();

  std::tuple<T, T, T, T> face;
  std::map<std::tuple<T, T, T, T>, T> faceMap;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerHex;

    std::vector<T> quad0 = {hexas[offset + 0], hexas[offset + 1], hexas[offset + 5], hexas[offset + 4]};
    std::vector<T> quad1 = {hexas[offset + 1], hexas[offset + 2], hexas[offset + 6], hexas[offset + 5]};
    std::vector<T> quad2 = {hexas[offset + 2], hexas[offset + 3], hexas[offset + 7], hexas[offset + 6]};
    std::vector<T> quad3 = {hexas[offset + 3], hexas[offset + 0], hexas[offset + 4], hexas[offset + 7]};
    std::vector<T> quad4 = {hexas[offset + 0], hexas[offset + 1], hexas[offset + 2], hexas[offset + 3]};
    std::vector<T> quad5 = {hexas[offset + 4], hexas[offset + 5], hexas[offset + 6], hexas[offset + 7]};

    std::list<std::vector<T>> quadList = {quad0, quad1, quad2, quad3, quad4, quad5};

    for(auto&& quad : quadList)
    {
      std::sort(quad.begin(), quad.end());
      face = std::make_tuple(quad[0], quad[1], quad[2], quad[3]);
      ++faceMap[face];
    }
  }

  typename std::map<std::tuple<T, T, T, T>, T>::iterator mapIter = faceMap.begin();

  while(mapIter != faceMap.end())
  {
    if((*mapIter).second > 1)
    {
      faceMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }

  faceList->getDataStore()->resizeTuples({faceMap.size()});
  auto& uFaces = *faceList;
  T index = 0;

  for(auto& pair : faceMap)
  {
    uFaces[4 * index] = std::get<0>(pair.first);
    uFaces[4 * index + 1] = std::get<1>(pair.first);
    uFaces[4 * index + 2] = std::get<2>(pair.first);
    uFaces[4 * index + 3] = std::get<3>(pair.first);
    ++index;
  }
}

/**
 * @brief Extracts all unique polygon edges.
 * @tparam T Specifies mesh-index type.
 * @param elemList Provides cyclic polygon vertex indexes.
 * @param edgeList Receives sorted two-index edges.
 */
template <typename T>
void Find2DElementEdges(const DataArray<T>* elemList, DataArray<T>* edgeList)
{
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
  auto& elems = *elemList;
  T v0 = 0;
  T v1 = 0;

  std::set<std::pair<T, T>> edgeSet;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerElem;

    for(usize j = 0; j < numVertsPerElem - 1; j++)
    {
      auto t0 = elems[offset + j];
      auto tj = elems[offset + j + 1];
      v0 = std::min(t0, tj);
      v1 = std::max(t0, tj);
      std::pair<T, T> edge = std::make_pair(v0, v1);
      edgeSet.insert(edge);
    }

    {
      usize j = numVertsPerElem - 1;
      auto t0 = elems[offset];
      auto tj = elems[offset + j];
      v0 = std::min(t0, tj);
      v1 = std::max(t0, tj);
      std::pair<T, T> edge = std::make_pair(v0, v1);
      edgeSet.insert(edge);
    }
  }

  typename std::set<std::pair<T, T>>::iterator setIter;
  edgeList->getDataStore()->resizeTuples({edgeSet.size()});
  auto& uEdges = *edgeList;
  T index = 0;

  for(auto& edge : edgeSet)
  {
    uEdges[2 * index] = edge.first;
    uEdges[2 * index + 1] = edge.second;
    ++index;
  }
}

/**
 * @brief Extracts polygon edges referenced exactly once.
 * @tparam T Specifies mesh-index type and reference-count type.
 * @param elemList Provides cyclic polygon vertex indexes.
 * @param edgeList Receives sorted boundary edges.
 */
template <typename T>
void Find2DUnsharedEdges(const DataArray<T>* elemList, DataArray<T>* edgeList)
{
  auto& elems = *elemList;
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
  T v0 = 0;
  T v1 = 0;

  std::map<std::pair<T, T>, T> edgeMap;

  for(usize i = 0; i < numElems; i++)
  {
    const usize offset = i * numVertsPerElem;

    for(usize j = 0; j < numVertsPerElem - 1; j++)
    {
      auto t0 = elems[offset + j];
      auto tj = elems[offset + j + 1];
      v0 = std::min(t0, tj);
      v1 = std::max(t0, tj);
      std::pair<T, T> edge = std::make_pair(v0, v1);
      ++edgeMap[edge];
    }

    {
      usize j = numVertsPerElem - 1;
      auto t0 = elems[offset];
      auto tj = elems[offset + j];
      v0 = std::min(t0, tj);
      v1 = std::max(t0, tj);
      std::pair<T, T> edge = std::make_pair(v0, v1);
      ++edgeMap[edge];
    }
  }

  typename std::map<std::pair<T, T>, T>::iterator mapIter = edgeMap.begin();

  while(mapIter != edgeMap.end())
  {
    if((*mapIter).second > 1)
    {
      edgeMap.erase(mapIter++);
    }
    else
    {
      ++mapIter;
    }
  }

  edgeList->getDataStore()->resizeTuples({edgeMap.size()});
  auto& bEdges = *edgeList;
  T index = 0;

  for(auto& edge : edgeMap)
  {
    bEdges[2 * index] = edge.first.first;
    bEdges[2 * index + 1] = edge.first.second;
    ++index;
  }
}
} // namespace Connectivity

/**
 * @namespace Topology
 * @brief Provides periodic-boundary and geometric-measure utilities.
 */
namespace Topology
{
/**
 * @typedef BoundingBoxFaces
 * @brief Defines a unique set of bounding-box faces.
 */
using BoundingBoxFaces = std::unordered_set<BoundingBox3Df::faces_enum>;

/**
 * @brief Finds bounding-box faces touched by selected vertices.
 *
 * Vertices must use the bounding box's coordinate system.
 * @param boundingBox Specifies spatial bounds.
 * @param vertices Provides flat XYZ coordinates.
 * @param vertexSet Specifies vertex indexes to test.
 * @return Unique touched faces.
 */
BoundingBoxFaces SIMPLNX_EXPORT FindElementPeriodicFaces(const BoundingBox3Df& boundingBox, const Float32AbstractDataStore& vertices, const std::set<IGeometry::MeshIndexType>& vertexSet);

/**
 * @brief Adjusts one centroid for periodic bounding-box faces.
 * @param boundingBox Specifies spatial bounds.
 * @param faces Specifies touched periodic faces.
 * @param centroids Provides and receives XYZ feature centroids.
 * @param featureId Specifies the centroid tuple.
 * @return True when the feature crosses a periodic boundary.
 *
 * Centroid coordinates must use the bounding box's coordinate system.
 */
bool SIMPLNX_EXPORT AdjustCentroidsForPeriodicFaces(const BoundingBox3Df& boundingBox, const BoundingBoxFaces& faces, Float32AbstractDataStore& centroids, IGeometry::MeshIndexType featureId);

/**
 * @brief Adjusts feature centroids from periodic ImageGeom range arrays.
 * @param imageGeom Specifies spatial bounds and dimensions.
 * @param xRanges Provides minimum and maximum X indexes per feature.
 * @param yRanges Provides minimum and maximum Y indexes per feature.
 * @param zRanges Provides minimum and maximum Z indexes per feature.
 * @param centroids Provides and receives XYZ feature centroids.
 * @return True when any feature crosses a periodic boundary.
 */
bool SIMPLNX_EXPORT AdjustCentroidsForPeriodicFaces(const ImageGeom& imageGeom, const UInt64AbstractDataStore& xRanges, const UInt64AbstractDataStore& yRanges, const UInt64AbstractDataStore& zRanges,
                                                    Float32AbstractDataStore& centroids);

/**
 * @brief Computes arithmetic vertex centroids for mesh elements.
 * @tparam T Specifies mesh-index type.
 * @param elemList Provides element vertex indexes.
 * @param vertices Provides flat XYZ coordinates.
 * @param centroids Receives one XYZ tuple per element.
 */
template <typename T>
void FindElementCentroids(const DataArray<T>* elemList, const Float32Array* vertices, Float32Array* centroids)
{
  auto& elems = *elemList;
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
  usize numDims = 3;
  auto& elementCentroids = *centroids;
  auto& vertex = *vertices;

  for(usize i = 0; i < numDims; i++)
  {
    for(usize j = 0; j < numElems; j++)
    {
      usize offset = j * numVertsPerElem;
      float32 vertPos = 0.0;
      for(usize k = 0; k < numVertsPerElem; k++)
      {
        vertPos += vertex[3 * elems[offset + k] + i];
      }
      vertPos /= static_cast<float32>(numVertsPerElem);
      elementCentroids[numDims * j + i] = vertPos;
    }
  }
}

/**
 * @brief Computes signed tetrahedron volumes.
 * @tparam T Specifies mesh-index type.
 * @param tetList Provides four vertex indexes per tetrahedron.
 * @param vertices Provides flat XYZ coordinates.
 * @param volumes Receives one signed volume per tetrahedron.
 */
template <typename T>
void FindTetVolumes(const DataArray<T>* tetList, const Float32Array* vertices, Float32Array* volumes)
{
  auto& tets = *tetList;
  const usize numTets = tetList->getNumberOfTuples();
  const usize numVertsPerTet = tetList->getNumberOfComponents();
  auto& vertex = *vertices;
  auto& volumePtr = *volumes;

  for(usize i = 0; i < numTets; i++)
  {
    const usize offset = i * numVertsPerTet;
    float32 vert0[3] = {vertex[3 * tets[offset + 0] + 0], vertex[3 * tets[offset + 0] + 1], vertex[3 * tets[offset + 0] + 2]};
    float32 vert1[3] = {vertex[3 * tets[offset + 1] + 0], vertex[3 * tets[offset + 1] + 1], vertex[3 * tets[offset + 1] + 2]};
    float32 vert2[3] = {vertex[3 * tets[offset + 2] + 0], vertex[3 * tets[offset + 2] + 1], vertex[3 * tets[offset + 2] + 2]};
    float32 vert3[3] = {vertex[3 * tets[offset + 3] + 0], vertex[3 * tets[offset + 3] + 1], vertex[3 * tets[offset + 3] + 2]};

    Eigen::Matrix3f vertMatrix;
    vertMatrix << vert1[0] - vert0[0], vert2[0] - vert0[0], vert3[0] - vert0[0], vert1[1] - vert0[1], vert2[1] - vert0[1], vert3[1] - vert0[1], vert1[2] - vert0[2], vert2[2] - vert0[2],
        vert3[2] - vert0[2];

    volumePtr[i] = (vertMatrix.determinant() / 6.0f);
  }
}

/**
 * @brief Computes signed hexahedron volumes from five tetrahedra.
 * @tparam T Specifies mesh-index type.
 * @param hexList Provides eight vertex indexes per hexahedron.
 * @param vertices Provides flat XYZ coordinates.
 * @param volumes Receives one signed volume per hexahedron.
 */
template <typename T>
void FindHexVolumes(const DataArray<T>* hexList, const Float32Array* vertices, Float32Array* volumes)
{
  const usize numHexas = hexList->getNumberOfTuples();
  const usize numElementsPerHex = hexList->getNumberOfComponents();
  auto& vertex = *vertices;
  auto& volumePtr = *volumes;
  auto& hexas = *hexList;

  for(usize i = 0; i < numHexas; i++)
  {
    // Sum signed volumes from a fixed five-tetrahedron decomposition.
    std::vector<std::vector<uint64>> subTets(5, std::vector<uint64>(4, 0));
    const usize offset = i * numElementsPerHex;

    // First tetrahedron from hexahedron vertices (0, 1, 3, 4);
    subTets[0][0] = hexas[offset + 0];
    subTets[0][1] = hexas[offset + 1];
    subTets[0][2] = hexas[offset + 3];
    subTets[0][3] = hexas[offset + 4];

    // Second tetrahedron from hexahedron vertices (1, 4, 5, 6);
    subTets[1][0] = hexas[offset + 1];
    subTets[1][1] = hexas[offset + 4];
    subTets[1][2] = hexas[offset + 5];
    subTets[1][3] = hexas[offset + 6];

    // Third tetrahedron from hexahedron vertices (1, 4, 6, 3);
    subTets[2][0] = hexas[offset + 1];
    subTets[2][1] = hexas[offset + 3];
    subTets[2][2] = hexas[offset + 6];
    subTets[2][3] = hexas[offset + 3];

    // Fourth tetrahedron from hexahedron vertices (1, 3, 6, 2);
    subTets[3][0] = hexas[offset + 1];
    subTets[3][1] = hexas[offset + 3];
    subTets[3][2] = hexas[offset + 6];
    subTets[3][3] = hexas[offset + 2];

    // Fifth tetrahedron from hexahedron vertices (3, 6, 7, 4);
    subTets[4][0] = hexas[offset + 3];
    subTets[4][1] = hexas[offset + 6];
    subTets[4][2] = hexas[offset + 7];
    subTets[4][3] = hexas[offset + 4];

    float32 volume = 0.0f;

    for(auto&& tet : subTets)
    {
      float32 vert0[3] = {vertex[3 * tet[0] + 0], vertex[3 * tet[0] + 1], vertex[3 * tet[0] + 2]};
      float32 vert1[3] = {vertex[3 * tet[1] + 0], vertex[3 * tet[1] + 1], vertex[3 * tet[1] + 2]};
      float32 vert2[3] = {vertex[3 * tet[2] + 0], vertex[3 * tet[2] + 1], vertex[3 * tet[2] + 2]};
      float32 vert3[3] = {vertex[3 * tet[3] + 0], vertex[3 * tet[3] + 1], vertex[3 * tet[3] + 2]};

      Eigen::Matrix3f vertMatrix;
      vertMatrix << vert1[0] - vert0[0], vert2[0] - vert0[0], vert3[0] - vert0[0], vert1[1] - vert0[1], vert2[1] - vert0[1], vert3[1] - vert0[1], vert1[2] - vert0[2], vert2[2] - vert0[2],
          vert3[2] - vert0[2];

      volume += (vertMatrix.determinant() / 6.0f);
    }

    volumePtr[i] = volume;
  }
}

/**
 * @brief Computes absolute areas for planar polygon elements.
 * @tparam T Specifies mesh-index type.
 * @param elemList Provides cyclic polygon vertex indexes.
 * @param vertices Provides flat XYZ coordinates.
 * @param areas Receives one area per element.
 */
template <typename T>
void Find2DElementAreas(const DataArray<T>* elemList, const Float32Array* vertices, Float32Array* areas)
{

  auto& elems = *elemList;
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = static_cast<int64_t>(elemList->getNumberOfComponents());
  if(numVertsPerElem < 3)
  {
    return;
  }
  auto& elemAreas = *areas;
  std::vector<Point3Df> coords;
  coords.reserve(numVertsPerElem);
  std::vector<float32> coordinate(3 * numVertsPerElem, 0.0f);

  for(usize i = 0; i < numElems; i++)
  {
    float32 area = 0.0f;
    const usize offset = i * numVertsPerElem;

    // Gather coordinates for normal and projection calculations.
    for(usize j = 0; j < numVertsPerElem; j++)
    {
      std::vector<float32> point{vertices->at(3 * elems[offset + j]), vertices->at(3 * elems[offset + j] + 1), vertices->at(3 * elems[offset + j] + 2)};
      coordinate.insert(coordinate.begin(), point.begin(), point.end());
      coords.emplace_back(vertices->at(3 * elems[offset + j]), vertices->at(3 * elems[offset + j] + 1), vertices->at(3 * elems[offset + j] + 2));
    }

    ZXZEuler normal = ZXZEuler(GeometryMath::FindPolygonNormal<float32>({coords.data(), coords.size()}).data());
    normal.normalize();

    float32 nx = (normal[0] > 0.0 ? normal[0] : -normal[0]);
    float32 ny = (normal[1] > 0.0 ? normal[1] : -normal[1]);
    float32 nz = (normal[2] > 0.0 ? normal[2] : -normal[2]);
    int32 projection = (nx > ny ? (nx > nz ? 0 : 2) : (ny > nz ? 1 : 2));

    float* coordinates = coordinate.data();
    for(int64 j = 0; j < numVertsPerElem; j++)
    {
      Point3D<float32> vert0(coordinates + (3 * j));
      Point3D<float32> vert1(coordinates + (3 * ((j + 1) % numVertsPerElem)));
      Point3D<float32> vert2(coordinates + (3 * ((j + 2) % numVertsPerElem)));

      switch(projection)
      {
      case 0: {
        area += coordinates[3 * ((j + 1) % numVertsPerElem) + 1] * (coordinates[3 * ((j + 2) % numVertsPerElem) + 2] - coordinates[3 * j + 2]);
        continue;
      }
      case 1: {
        area += coordinates[3 * ((j + 1) % numVertsPerElem) + 0] * (coordinates[3 * ((j + 2) % numVertsPerElem) + 2] - coordinates[3 * j + 2]);
        continue;
      }
      case 2: {
        area += coordinates[3 * ((j + 1) % numVertsPerElem) + 0] * (coordinates[3 * ((j + 2) % numVertsPerElem) + 1] - coordinates[3 * j + 1]);
        continue;
      }
      default:
        break;
      }
    }

    switch(projection)
    {
    case 0: {
      area /= (2.0f * nx);
      break;
    }
    case 1: {
      area /= (2.0f * ny);
      break;
    }
    case 2: {
      area /= (2.0f * nz);
    }
    default:
      break;
    }
    elemAreas[i] = fabsf(area);
  }
}
} // namespace Topology
} // namespace nx::core::GeometryHelpers

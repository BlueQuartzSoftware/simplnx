#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/EulerAngle.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include <Eigen/Dense>

#include <xtensor/xio.hpp>

namespace nx::core
{
namespace GeometryHelpers
{
using ErrorCode = int32;

namespace Description
{

/**
 * @brief Generates a string description for the given arguments
 * @param geometry The geometry to generate the information string
 * @param dims The dimensions of the image geometry
 * @param spacing The spacing of the image geometry
 * @param origin The origin of the image geomtry
 * @return
 */
SIMPLNX_EXPORT std::string GenerateGeometryInfo(const nx::core::SizeVec3& dims, const nx::core::FloatVec3& spacing, const nx::core::FloatVec3& origin, IGeometry::LengthUnit units);

} // namespace Description

namespace Connectivity
{
/**
 * @brief
 * @tparam T
 * @tparam K
 * @param elemList
 * @param dynamicList
 * @param numVerts
 */
template <typename T, typename K>
void FindElementsContainingVert(const DataArray<K>* elemList, DynamicListArray<T, K>* dynamicList, usize numVerts)
{
  DataStructure* dataStructure = dynamicList->getDataStructure();
  auto parentId = dynamicList->getParentIds().front();

  auto& elems = *elemList;
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();

  // Allocate the basic structures
  std::vector<T> linkCount(numVerts, 0);

  // Fill out lists with number of references to cells
  std::vector<K> linkLoc(numVerts, static_cast<K>(0));
  K* verts = nullptr;

  // vtkPolyData *pdata = static_cast<vtkPolyData *>(data);
  // Traverse data to determine number of uses of each point
  for(usize elemId = 0; elemId < numElems; elemId++)
  {
    usize offset = elemId * numVertsPerElem;
    for(usize j = 0; j < numVertsPerElem; j++)
    {
      linkCount[elems[offset + j]]++;
    }
  }

  // Now allocate storage for the links
  dynamicList->allocateLists(linkCount);

  for(usize elemId = 0; elemId < numElems; elemId++)
  {
    usize offset = elemId * numVertsPerElem;
    for(usize j = 0; j < numVertsPerElem; j++)
    {
      dynamicList->insertCellReference(elems[offset + j], (linkLoc[elems[offset + j]])++, elemId);
    }
  }
}

/**
 * @brief
 * @tparam T
 * @tparam K
 * @param elemList
 * @param elemsContainingVert
 * @param dynamicList
 * @param geometryType
 * @return int32
 */
template <typename T, typename K>
ErrorCode FindElementNeighbors(const DataArray<K>* elemList, const DynamicListArray<T, K>* elemsContainingVert, DynamicListArray<T, K>* dynamicList, IGeometry::Type geometryType)
{
  DataStructure* dataStructure = dynamicList->getDataStructure();
  auto parentId = dynamicList->getParentIds().front();
  auto& elems = *elemList;
  const usize numElems = elemList->getNumberOfTuples();
  const usize numVertsPerElem = elemList->getNumberOfComponents();
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

  // Allocate an array of bools that we use each iteration so that we don't put duplicates into the array
  std::vector<uint8> visited(numElems, 0);

  // Reuse this vector for each loop. Avoids re-allocating the memory each time through the loop
  std::vector<K> loop_neighbors(32, 0);

  std::cout << elems.getDataStoreRef().xarray() << std::endl;

  // Build up the element adjacency list now that we have the element links
  for(usize t = 0; t < numElems; ++t)
  {
    //   qDebug() << "Analyzing Cell " << t << "\n";
    const usize offset = t * numVertsPerElem;
    for(usize v = 0; v < numVertsPerElem; ++v)
    {
      //   qDebug() << " vert " << v << "\n";
      T nEs = elemsContainingVert->getNumberOfElements(elems[offset + v]);
      K* vertIdxs = elemsContainingVert->getElementListPointer(elems[offset + v]);

      for(T vt = 0; vt < nEs; ++vt)
      {
        if(vertIdxs[vt] == static_cast<K>(t))
        {
          continue;
        } // This is the same element as our "source"
        if(visited[vertIdxs[vt]])
        {
          continue;
        } // We already added this element so loop again
        //      qDebug() << "   Comparing Element " << vertIdxs[vt] << "\n";
        const auto vertId = vertIdxs[vt];
        auto vertCell = elemList->cbegin() + (vertIdxs[vt] * elemList->getNumberOfComponents());
        usize vCount = 0;
        // Loop over all the vertex indices of this element and try to match numSharedVerts of them to the current loop element
        // If there is numSharedVerts match then that element is a neighbor of the source. If there are more than numVertsPerElem
        // matches then there is a real problem with the mesh and the program is going to return an error.
        for(usize i = 0; i < numVertsPerElem; i++)
        {
          for(usize j = 0; j < numVertsPerElem; j++)
          {
            if(elems[offset + i] == *(vertCell + j))
            {
              vCount++;
            }
          }
        }

        // So if our vertex match count is numSharedVerts and we have not visited the element in question then add this element index
        // into the list of vertex indices as neighbors for the source element.
        if(vCount == numSharedVerts)
        {
          // qDebug() << "       Neighbor: " << vertIdxs[vt] << "\n";
          // Use the current count of neighbors as the index
          // into the loop_neighbors vector and place the value of the vertex element at that index
          loop_neighbors[linkCount[t]] = vertIdxs[vt];
          linkCount[t]++; // Increment the count for the next time through
          if(linkCount[t] >= loop_neighbors.size())
          {
            loop_neighbors.resize(loop_neighbors.size() + 10);
          }
          visited[vertIdxs[vt]] = true; // Set this element as visited so we do NOT add it again
        }
      }
    }
    // Reset all the visited cell indexs back to false (zero)
    for(int64 k = 0; k < linkCount[t]; ++k)
    {
      visited[loop_neighbors[k]] = false;
    }
    // Allocate the array storage for the current edge to hold its vertex list
    dynamicList->setElementList(t, linkCount[t], &(loop_neighbors[0]));
  }

  return err;
}

/**
 * @brief
 * @tparam T
 * @param tetList
 * @param edgeList
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
 * @brief
 * @tparam T
 * @param hexList
 * @param edge_List
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
 * @brief
 * @tparam T
 * @param tetList
 * @param faceList
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
 * @brief
 * @tparam T
 * @param hexList
 * @param faceList
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
 * @brief
 * @tparam T
 * @param tetList
 * @param edgeList
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
      edgeMap[edge]++;
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
 * @brief
 * @tparam T
 * @param hexList
 * @param edge_List
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
      edgeMap[edge]++;
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
 * @brief
 * @tparam T
 * @param tetList
 * @param faceList
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
      faceMap[face]++;
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
 * @brief
 * @tparam T
 * @param hexList
 * @param faceList
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
      faceMap[face]++;
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
 * @brief
 * @tparam T
 * @param elemList
 * @param edgeList
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

    for(usize j = 0; j < numVertsPerElem; j++)
    {
      if(j == (numVertsPerElem - 1))
      {
        if(elems[offset + j] > elems[offset + 0])
        {
          v0 = elems[offset + 0];
          v1 = elems[offset + j];
        }
        else
        {
          v0 = elems[offset + j];
          v1 = elems[offset + 0];
        }
      }
      else
      {
        if(elems[offset + j] > elems[offset + j + 1])
        {
          v0 = elems[offset + j + 1];
          v1 = elems[offset + j];
        }
        else
        {
          v0 = elems[offset + j];
          v1 = elems[offset + j + 1];
        }
      }
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
 * @brief
 * @tparam T
 * @param elemList
 * @param edgeList
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

    for(usize j = 0; j < numVertsPerElem; j++)
    {
      if(j == (numVertsPerElem - 1))
      {
        if(elems[offset + j] > elems[offset + 0])
        {
          v0 = elems[offset + 0];
          v1 = elems[offset + j];
        }
        else
        {
          v0 = elems[offset + j];
          v1 = elems[offset + 0];
        }
      }
      else
      {
        if(elems[offset + j] > elems[offset + j + 1])
        {
          v0 = elems[offset + j + 1];
          v1 = elems[offset + j];
        }
        else
        {
          v0 = elems[offset + j];
          v1 = elems[offset + j + 1];
        }
      }
      std::pair<T, T> edge = std::make_pair(v0, v1);
      edgeMap[edge]++;
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

namespace Topology
{
/**
 * @brief
 * @tparam T
 * @param elemList
 * @param vertices
 * @param centroids
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
 * @brief
 * @tparam T
 * @param tetList
 * @param vertices
 * @param volumes
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
 * @brief
 * @tparam T
 * @param hexList
 * @param vertices
 * @param volumes
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
    // Subdivide each hexahedron into 5 tetrahedra & sum their volumes
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
 * @brief
 * @tparam T
 * @param elemList
 * @param vertices
 * @param areas
 */
template <typename T>
void Find2DElementAreas(const DataArray<T>* elemList, const Float32Array* vertices, Float32Array* areas)
{
  float32 nx, ny, nz;
  int32 projection;

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

    // Create a contiguous vertex coordinates list
    // This simplifies the pointer arithmetic a bit
    for(usize j = 0; j < numVertsPerElem; j++)
    {
      std::vector<float32> point{vertices->at(3 * elems[offset + j]), vertices->at(3 * elems[offset + j] + 1), vertices->at(3 * elems[offset + j] + 2)};
      coordinate.insert(coordinate.begin(), point.begin(), point.end());
      coords.emplace_back(vertices->at(3 * elems[offset + j]), vertices->at(3 * elems[offset + j] + 1), vertices->at(3 * elems[offset + j] + 2));
    }

    ZXZEuler normal = ZXZEuler(GeometryMath::FindPolygonNormal<float32>({coords.data(), coords.size()}).data());
    normal.normalize();

    nx = (normal[0] > 0.0 ? normal[0] : -normal[0]);
    ny = (normal[1] > 0.0 ? normal[1] : -normal[1]);
    nz = (normal[2] > 0.0 ? normal[2] : -normal[2]);
    projection = (nx > ny ? (nx > nz ? 0 : 2) : (ny > nz ? 1 : 2));

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
    }
    elemAreas[i] = fabsf(area);
  }
}
} // namespace Topology
} // namespace GeometryHelpers
} // namespace nx::core

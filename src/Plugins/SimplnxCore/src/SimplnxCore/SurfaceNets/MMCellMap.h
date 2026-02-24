// MMCellMap.h
//
// Interface for MMCellMaps, which are used to process the sampled data to be surfaced
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_CELL_MAP_H
#define MM_CELL_MAP_H

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include "MMCellFlag.h"
#include "MMSurfaceNet.h"

#include <array>
#include <vector>

class MMCellMap
{
public:
  // Basic cell map containing tissue-type labels
  MMCellMap(TriangleGeom::SharedVertexList::store_type& verticesStore, Int32Array* labels, size_t arraySize[3], const float voxelSize[3]);

  ~MMCellMap() = default;

  bool init();
  bool valid() const;

  // Relax vertex positions using relaxation attributes or reset to cell centers
  void relax(const MMSurfaceNet::RelaxAttrs& relaxAttrs) const;

  // Data for export
  void getArraySize(int arraySize[3]) const;
  void getVoxelSize(float voxelSize[3]) const;
  size_t numVertices() const;
  size_t numEdgeCrossings() const;
  MMCellFlag::VertexType vertexType(size_t vertexIndex) const;
  bool getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, size_t quadVtxIndices[4], int32_t quadLabels[2], size_t quadNxArrayIndices[2]) const;
  void getVertexPosition(size_t vertexIndex, float position[3]) const;

  MMCellFlag::VertexType cellVertexType(size_t cellArrayIdx) const;

  // SoA accessors for external code that needs flag/vertex data by cell index
  uint32_t cellFlag(size_t cellArrayIdx) const;
  size_t cellVertexIndex(size_t cellArrayIdx) const;

  int32_t label(const int32 cellIndex[3]) const;

  void getVertexCellIndex(size_t vertexIndex, int cellIndex[3]) const;
  size_t cellArrayIndex(const int cellIndex[3]) const;

private:
  std::array<size_t, 3> m_arraySize = {0, 0, 0};
  std::array<float, 3> m_voxelSize = {1.0f, 1.0f, 1.0f};
  std::array<size_t, 3> m_NxDims = {0, 0, 0};

  // SoA cell data: separate dense arrays for flags and vertex indices
  std::vector<uint32_t> m_flagArray;
  std::vector<size_t> m_vertexIndexArray;

  // Vertex array: stores flat cell index for each vertex
  struct Vertex
  {
    size_t cellFlatIndex;
  };
  std::vector<Vertex> m_VertexArray;

  TriangleGeom::SharedVertexList::store_type& m_VerticesStoreRef;
  Int32Array* m_NxLabelsPtr = nullptr;

  // Cached edge crossing count (computed during setCellVertices)
  size_t m_numEdgeCrossings = 0;

  bool setCellVertices();

  // Recover (i,j,k) from flat cell index
  void cellIndexFromFlat(size_t flatIdx, int& i, int& j, int& k) const;

  // Access cell map
  size_t cellArrayIndex(int i, int j, int k) const;
  bool isEdgeCrossing(size_t cellArrayIdx, MMCellFlag::Edge edge) const;
  void getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2], size_t quadNxArrayIndices[2]) const;
  void getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const;

  usize getNxCellArrayIndex(size_t vertexIndex) const;

  // Access vertex data
  void getVertexPosition(const int cellIndex[3], float position[3]) const;

  // Access cell neighbors — returns flat cell index of neighbor and fills nbrCellIndex
  size_t getFaceNeighborAndIndex(const int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]) const;
};

#endif

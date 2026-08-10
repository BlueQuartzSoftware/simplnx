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

class MMCellMap
{
public:
  // Basic cell map containing tissue-type labels
  MMCellMap(TriangleGeom::SharedVertexList::store_type& verticesStore, Int32Array* labels, size_t arraySize[3], const float voxelSize[3]);

  ~MMCellMap();

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
  bool getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], int32_t quadLabels[2], size_t quadNxArrayIndices[2]);
  bool getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, size_t quadVtxIndices[4], int32_t quadLabels[2], size_t quadNxArrayIndices[2]) const;
  void getVertexPosition(size_t vertexIndex, float position[3]) const;

  MMCellFlag::VertexType cellVertexType(size_t cellArrayIndex) const;

  // Node type in the shared simplnx NodeType convention: min(distinct corner labels, 4),
  // plus 10 when any corner is exterior padding. This is the same quantity QuickSurfaceMesh
  // computes from its per-node owner list, over the same 8 surrounding voxels.
  int8_t nodeType(const int cellIndex[3]) const;

  struct Vertex
  {
    int32_t cellIndex[3];
  };

  struct Cell
  {
    size_t vertexIndex;
    MMCellFlag flag;
  };

  int32_t label(const int32 cellIndex[3]) const;

  Cell* getCell(int cellIndex[3]) const;
  Cell* getCell(int i, int j, int k) const;
  Cell* getCell(size_t cellArrayIndex) const;

  void getVertexCellIndex(size_t vertexIndex, int cellIndex[3]) const;
  size_t cellArrayIndex(const int cellIndex[3]) const;

private:
  std::array<size_t, 3> m_arraySize = {0, 0, 0};
  std::array<float, 3> m_voxelSize = {1.0f, 1.0f, 1.0f};
  std::array<size_t, 3> m_NxDims = {0, 0, 0};

  Cell* m_cellArray;

  std::vector<Vertex> m_VertexArray;
  TriangleGeom::SharedVertexList::store_type& m_VerticesStoreRef;
  Int32Array* m_NxLabelsPtr = nullptr;

  bool setCellVertices();

  // Access cell map
  size_t cellArrayIndex(int i, int j, int k) const;
  void getCornerLabels(const int cellIndex[3], int32_t labels[8]) const;
  bool isEdgeCrossing(size_t cellArrayIndex, MMCellFlag::Edge edge) const;
  void getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2], size_t quadNxArrayIndices[2]) const;
  void getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12]) const;
  void getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const;

  usize getNxCellArrayIndex(size_t vertexIndex) const;

  // Access vertex data
  void getVertexPosition(const int cellIndex[3], float position[3]) const;
  void getVertexPosition(int i, int j, int k, float position[3]) const;
  usize vertexFaceNeighborVertexIndex(size_t vertexIndex, MMCellFlag::Face face) const;

  // Access cell neighbors
  Cell* getFaceNeighborCellAndIndex(const int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]) const;
};

#endif

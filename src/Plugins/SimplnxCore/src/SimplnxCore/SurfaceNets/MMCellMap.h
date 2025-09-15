// MMCellMap.h
//
// Interface for MMCellMaps, which are used to process the sampled data to be surfaced
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_CELL_MAP_H
#define MM_CELL_MAP_H

#include "simplnx/DataStructure/DataArray.hpp"

#include "MMCellFlag.h"
#include "MMSurfaceNet.h"

#include <array>

class MMCellMap
{
public:
  // Basic cell map containing tissue-type labels
  MMCellMap(int arraySize[3], float voxelSize[3]);

  ~MMCellMap();

  bool init(Int32Array& labels);
  bool valid() const;

  // Relax vertex positions using relaxation attributes or reset to cell centers
  void relax(const MMSurfaceNet::RelaxAttrs& relaxAttrs);
  void reset() const;

  // Data for export
  void getArraySize(int arraySize[3]) const;
  void getVoxelSize(float voxelSize[3]) const;
  size_t numVertices() const;
  size_t numEdgeCrossings() const;
  MMCellFlag::VertexType vertexType(size_t vertexIndex) const;
  bool getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], int32_t quadLabels[2], size_t quadNxArrayIndices[2]);
  bool getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, size_t quadVtxIndices[4], int32_t quadLabels[2], size_t quadNxArrayIndices[2]);
  void getVertexPosition(size_t vertexIndex, float position[3]);

  MMCellFlag::VertexType cellVertexType(size_t cellArrayIndex) const;

  struct Cell
  {
    int32_t label;
    float vertexOffset[3];
    size_t vertexIndex;
    MMCellFlag flag;
  };

  struct Vertex
  {
    int32_t cellIndex[3];
  };

  Cell* getCell(int cellIndex[3]) const;
  Cell* getCell(int i, int j, int k) const;
  Cell* getCell(size_t cellArrayIndex) const;

  void getVertexCellIndex(size_t vertexIndex, int cellIndex[3]) const;
  size_t cellArrayIndex(int cellIndex[3]) const;

private:
  std::array<int32_t, 3> m_arraySize = {0, 0, 0};
  std::array<float, 3> m_voxelSize = {1.0f, 1.0f, 1.0f};

  std::array<int32_t, 3> m_NxDims = {0, 0, 0};
  Cell* m_cellArray;

  size_t m_numVertices;
  Vertex* m_vertices;
  bool setCellVertices();

  // Access cell map

  size_t cellArrayIndex(int i, int j, int k) const;
  void getCellLabels(Cell* cell, int32_t labels[8]) const;
  bool isEdgeCrossing(size_t cellArrayIndex, MMCellFlag::Edge edge) const;
  void getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2], size_t quadNxArrayIndices[2]);
  void getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12]);
  void getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const;

  usize getNxCellArrayIndex(int64_t vertexIndex);

  // Access vertex data
  void getVertexPosition(int cellIndex[3], float position[3]) const;
  void getVertexPosition(int i, int j, int k, float position[3]) const;
  int vertexFaceNeighborVertexIndex(size_t vertexIndex, MMCellFlag::Face face) const;

  // Access cell neighbors
  Cell* getFaceNeighborCellAndIndex(int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]);
};

#endif

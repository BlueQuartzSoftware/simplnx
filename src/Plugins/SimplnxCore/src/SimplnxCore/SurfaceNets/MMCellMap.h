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

  void init(Int32Array& labels);

  // Relax vertex positions using relaxation attributes or reset to cell centers
  void relax(MMSurfaceNet::RelaxAttrs relaxAttrs);
  void reset();

  // Data for export
  void getArraySize(int arraySize[3]);
  void getVoxelSize(float voxelSize[3]);
  int numVertices();
  int numEdgeCrossings();
  MMCellFlag::VertexType vertexType(int vertexIndex);
  bool getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], int32_t quadLabels[2], usize quadNxArrayIndices[2]);
  bool getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, int quadVtxIndices[4], int32_t quadLabels[2], usize quadNxArrayIndices[2]);
  void getVertexPosition(int vertexIndex, float position[3]);

  MMCellFlag::VertexType cellVertexType(int cellArrayIndex);
  struct Cell
  {
    int32_t label;
    MMCellFlag flag;
    int vertexIndex;
    float vertexOffset[3];
  };

  struct Vertex
  {
    int cellIndex[3];
  };

  Cell* getCell(int cellIndex[3]);
  Cell* getCell(int i, int j, int k);
  Cell* getCell(int cellArrayIndex);

  void getVertexCellIndex(int vertexIndex, int cellIndex[3]);
  int cellArrayIndex(int cellIndex[3]);

private:
  std::array<int32_t, 3> m_arraySize = {0, 0, 0};
  std::array<float, 3> m_voxelSize = {1.0f, 1.0f, 1.0f};

  std::array<int32_t, 3> m_NxDims = {0, 0, 0};
  Cell* m_cellArray;

  int m_numVertices;
  Vertex* m_vertices;
  void setCellVertices();

  // Access cell map

  int cellArrayIndex(int i, int j, int k);
  void getCellLabels(Cell* cell, int32_t labels[8]);
  bool isEdgeCrossing(int cellArrayIndex, MMCellFlag::Edge edge);
  void getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2], usize quadNxArrayIndices[2]);
  void getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12]);
  void getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, int quadVtxIndices[4]);

  usize getNxCellArrayIndex(int vertexIndex);

  // Access vertex data
  void getVertexPosition(int cellIndex[3], float position[3]);
  void getVertexPosition(int i, int j, int k, float position[3]);
  int vertexFaceNeighborVertexIndex(int vertexIndex, MMCellFlag::Face face);

  // Access cell neighbors
  Cell* getFaceNeighborCellAndIndex(int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]);
};

#endif

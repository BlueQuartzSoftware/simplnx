// MMCellMap.h
//
// Interface for MMCellMaps, which are used to process the sampled data to be surfaced
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_CELL_MAP_H
#define MM_CELL_MAP_H

#include "MMCellFlag.h"
#include "MMSurfaceNet.h"

class MMCellMap
{
public:
  // Basic cell map containing tissue-type labels
  MMCellMap(unsigned short* labels, int arraySize[3], float voxelSize[3]);
  ~MMCellMap();

  // Relax vertex positions using relaxation attributes or reset to cell centers
  void relax(MMSurfaceNet::RelaxAttrs relaxAttrs);
  void reset();

  // Data for export
  void getArraySize(int arraySize[3]);
  void getVoxelSize(float voxelSize[3]);
  int numVertices();
  int numEdgeCrossings();
  MMCellFlag::VertexType vertexType(int vertexIndex);
  bool getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], unsigned short quadLabels[2]);
  bool getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, int quadVtxIndices[4], unsigned short quadLabels[2]);
  void getVertexPosition(int vertexIndex, float position[3]);

  MMCellFlag::VertexType cellVertexType(int cellArrayIndex);
  struct Cell
  {
    unsigned short label;
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
  // Use of C-style arrays. C-style arrays are used deliberately for cell indices,
  // vertex positions, cells in the cell map, vertices, etc. This was done after
  // careful comparison of SurfaceNet construction and relaxation speeds with C-style
  // arrays vs. std library vectors and arrays. Although the latter approach is
  // encouraged with C++ and simplifies some implementation details, it was found
  // to negatively impact construction and relaxation times in both Debug and Release
  // modes in Visual Studio 2015.
  int m_arraySize[3];
  float m_voxelSize[3];

  Cell* m_cellArray;

  int m_numVertices;
  Vertex* m_vertices;
  void initCell(Cell* cell, unsigned short label);
  void setCellVertices();

  // Access cell map

  int cellArrayIndex(int i, int j, int k);
  void getCellLabels(Cell* cell, unsigned short labels[8]);
  bool isEdgeCrossing(int cellArrayIndex, MMCellFlag::Edge edge);
  void getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, unsigned short quadLabels[2]);
  void getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12]);
  void getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, int quadVtxIndices[4]);

  // Access vertex data
  void getVertexPosition(int cellIndex[3], float position[3]);
  void getVertexPosition(int i, int j, int k, float position[3]);
  int vertexFaceNeighborVertexIndex(int vertexIndex, MMCellFlag::Face face);

  // Access cell neighbors
  Cell* getFaceNeighborCellAndIndex(int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]);
};

#endif

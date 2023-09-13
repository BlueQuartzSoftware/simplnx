// MMCellMap.cpp
//
// MMCellMap implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <cstdlib>
#include <exception>
#include <iostream>

#include "MMCellMap.h"
#include "MMSurfaceNet.h"

// Basic cell map containing material labels
MMCellMap::MMCellMap(int32_t* labels, int arraySize[3], float voxelSize[3])
: m_cellArray(NULL)
, m_numVertices(0)
, m_vertices(NULL)
{
  // Allocate memory for the cell map. To ensure closed shapes and sharp corners
  // and edges at volume faces, faces are padded by one voxel with a reserved
  // label.
  for(int i = 0; i < 3; i++)
  {
    m_arraySize[i] = arraySize[i] + 2;
    m_voxelSize[i] = voxelSize[i];
  }
  int numCells = m_arraySize[0] * m_arraySize[1] * m_arraySize[2];
  try
  {
    m_cellArray = new Cell[numCells];
  } catch(std::bad_alloc& ba)
  {
    m_cellArray = NULL;
    return;
  }

  // Initialize interior cell contents. Each cell stores the label of it's bottom, left, back
  // corner.
  Cell* pCell = m_cellArray;
  int32_t* pLabel = labels;
  int32_t padLabel = (int32_t)MMSurfaceNet::ReservedLabel::Padding;
  for(int k = 0; k < m_arraySize[2]; k++)
  {
    for(int j = 0; j < m_arraySize[1]; j++)
    {
      for(int i = 0; i < m_arraySize[0]; i++)
      {
        if(i == 0 || i == m_arraySize[0] - 1 || j == 0 || j == m_arraySize[1] - 1 || k == 0 || k == m_arraySize[2] - 1)
        {
          initCell(pCell++, padLabel);
        }
        else
        {
          initCell(pCell++, *pLabel++);
        }
      }
    }
  }

  // Set the cell vertices
  setCellVertices();
}
MMCellMap::~MMCellMap()
{
  if(m_cellArray)
    delete[] m_cellArray;
  if(m_vertices)
    delete[] m_vertices;
}

// Relax vertex positions using relaxation attributes or reset to cell centers
void MMCellMap::relax(MMSurfaceNet::RelaxAttrs relaxAttrs)
{
  for(int i = 0; i < relaxAttrs.numRelaxIterations; i++)
  {
    for(int idxVtx = 0; idxVtx < m_numVertices; idxVtx++)
    {
      int cellIdx[3];
      getVertexCellIndex(idxVtx, cellIdx);
      Cell* pCell = getCell(cellIdx);

      int numNeighbors = 0;
      float avgP[3] = {0.0f, 0.0f, 0.0f};
      if(pCell->flag.vertexType() == MMCellFlag::VertexType::SurfaceVertex)
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(pCell->flag.faceCrossingType(face) != MMCellFlag::FaceCrossingType::NoFaceCrossing)
          {
            int nbrIdx[3];
            Cell* nbrCell = getFaceNeighborCellAndIndex(cellIdx, face, nbrIdx);
            avgP[0] += nbrCell->vertexOffset[0] + nbrIdx[0] - cellIdx[0];
            avgP[1] += nbrCell->vertexOffset[1] + nbrIdx[1] - cellIdx[1];
            avgP[2] += nbrCell->vertexOffset[2] + nbrIdx[2] - cellIdx[2];
            numNeighbors++;
          }
        }
      }
      else
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(pCell->flag.faceCrossingType(face) == MMCellFlag::FaceCrossingType::JunctionFaceCrossing)
          {
            int nbrIdx[3];
            Cell* nbrCell = getFaceNeighborCellAndIndex(cellIdx, face, nbrIdx);
            avgP[0] += nbrCell->vertexOffset[0] + nbrIdx[0] - cellIdx[0];
            avgP[1] += nbrCell->vertexOffset[1] + nbrIdx[1] - cellIdx[1];
            avgP[2] += nbrCell->vertexOffset[2] + nbrIdx[2] - cellIdx[2];
            numNeighbors++;
          }
        }
      }

      // Add a fraction of the averaged vertex position to the current position
      float* p = pCell->vertexOffset;
      if(numNeighbors > 0)
      {
        avgP[0] /= (float)numNeighbors;
        avgP[1] /= (float)numNeighbors;
        avgP[2] /= (float)numNeighbors;
        float alpha = relaxAttrs.relaxFactor;
        p[0] = (1.0 - alpha) * p[0] + alpha * avgP[0];
        p[1] = (1.0 - alpha) * p[1] + alpha * avgP[1];
        p[2] = (1.0 - alpha) * p[2] + alpha * avgP[2];

        // Constrain vertex location to a max distance from the original voxel
        float min = 0.5 - relaxAttrs.maxDistFromCellCenter;
        float max = 0.5 + relaxAttrs.maxDistFromCellCenter;
        if(p[0] < min)
          p[0] = min;
        if(p[0] > max)
          p[0] = max;
        if(p[1] < min)
          p[1] = min;
        if(p[1] > max)
          p[1] = max;
        if(p[2] < min)
          p[2] = min;
        if(p[2] > max)
          p[2] = max;
      }
    }
  }
}
void MMCellMap::reset()
{
  for(int idxVtx = 0; idxVtx < m_numVertices; idxVtx++)
  {
    int cellIdx[3];
    getVertexCellIndex(idxVtx, cellIdx);
    Cell* pCell = getCell(cellIdx);
    pCell->vertexOffset[0] = 0.5f;
    pCell->vertexOffset[1] = 0.5f;
    pCell->vertexOffset[2] = 0.5f;
  }
}

// Data for export
void MMCellMap::getArraySize(int arraySize[3])
{
  arraySize[0] = m_arraySize[0];
  arraySize[1] = m_arraySize[1];
  arraySize[2] = m_arraySize[2];
}
void MMCellMap::getVoxelSize(float voxelSize[3])
{
  voxelSize[0] = m_voxelSize[0];
  voxelSize[1] = m_voxelSize[1];
  voxelSize[2] = m_voxelSize[2];
}
int MMCellMap::numVertices()
{
  return m_numVertices;
}
int MMCellMap::numEdgeCrossings()
{
  int cellMapIdx = 0;
  int numCrossings = 0;
  for(int k = 0; k < m_arraySize[2]; k++)
  {
    for(int j = 0; j < m_arraySize[1]; j++)
    {
      for(int i = 0; i < m_arraySize[0]; i++)
      {
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::LeftBackEdge))
          numCrossings++;
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::LeftBottomEdge))
          numCrossings++;
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::BackBottomEdge))
          numCrossings++;
        cellMapIdx++;
      }
    }
  }
  return numCrossings;
}
MMCellFlag::VertexType MMCellMap::vertexType(int vertexIndex)
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  return (cellVertexType(cellArrayIndex(cellIndex)));
}
// Returns true if there is an edge crossing and false otherwise. If there is an edge
// crossing, we defince a surface quad from vertices in the 4 cells touching the edge.
// The four positions of the quad's corner vertices are inserted into quadCorners as
// [x0, y0, z0, x1, y1 ...] in clockwise order and the quad face labels are inserted
// into quadLabels as [labelTopFaceOfQuad, labelBottomFaceOfQuad]. If there is no edge
// crossing, quadCorners and quadLabels will not be set.
bool MMCellMap::getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], int32_t quadLabels[2])
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  if(!isEdgeCrossing(cellArrayIndex(cellIndex), edge))
  {
    return false;
  }

  // Because there is an edge crossing, cell map access in the following will be
  // in-bounds by construction of the cell map.
  getEdgeLabels(cellIndex, edge, quadLabels);
  getEdgeQuadPositions(cellIndex, edge, quadCorners);
  return true;
}
// Returns true if there is an edge crossing and false otherwise. If there is an edge
// crossing, we defince a surface quad from vertices in the 4 cells touching the edge.
// The indices of these 4 vertices are inserted into quadVtxIndices in clockwise order
// and the quad face labels are inserted into quadLabels as [labelTopFaceOfQuad,
// labelBottomFaceOfQuad]. If there is no edge crossing, quadCorners and quadLabels
// will not be set.
bool MMCellMap::getEdgeQuad(int vertexIndex, MMCellFlag::Edge edge, int quadVtxIndices[4], int32_t quadLabels[2])
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  if(!isEdgeCrossing(cellArrayIndex(cellIndex), edge))
  {
    return false;
  }

  // Because there is an edge crossing, cell map access in the following will be
  // in-bounds by construction of the cell map.
  getEdgeLabels(cellIndex, edge, quadLabels);
  getEdgeQuadVtxIndices(cellIndex, edge, quadVtxIndices);
  return true;
}

void MMCellMap::getVertexPosition(int vertexIndex, float position[3])
{
  getVertexPosition(m_vertices[vertexIndex].cellIndex, position);
}

void MMCellMap::initCell(Cell* cell, int32_t label)
{
  cell->label = label;
  cell->flag.clear();
  cell->vertexIndex = -1;
  cell->vertexOffset[0] = 0.5f;
  cell->vertexOffset[1] = 0.5f;
  cell->vertexOffset[2] = 0.5f;
}

void MMCellMap::setCellVertices()
{
  // Set cell type and count cell vertices. There are no vertices in right, front,
  // top faces.
  m_numVertices = 0;
  for(int k = 0; k < m_arraySize[2] - 1; k++)
  {
    for(int j = 0; j < m_arraySize[1] - 1; j++)
    {
      for(int i = 0; i < m_arraySize[0] - 1; i++)
      {
        Cell* pCell = getCell(i, j, k);
        int32_t cellLabels[8];
        getCellLabels(pCell, cellLabels);
        pCell->flag.set(cellLabels);
        if(pCell->flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          m_numVertices++;
        }
      }
    }
  }

  // Create cell vertices. There are no vertices in right, front, top faces.
  try
  {
    if(m_vertices != NULL)
      delete[] m_vertices;
    m_vertices = new Vertex[m_numVertices];
  } catch(std::bad_alloc& ba)
  {
    delete[] m_cellArray;
    m_cellArray = NULL;
    m_numVertices = 0;
    m_vertices = NULL;
    return;
  }
  int idxVtx = 0;
  for(int k = 0; k < m_arraySize[2] - 1; k++)
  {
    for(int j = 0; j < m_arraySize[1] - 1; j++)
    {
      for(int i = 0; i < m_arraySize[0] - 1; i++)
      {
        Cell* pCell = getCell(i, j, k);
        if(pCell->flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          pCell->vertexIndex = idxVtx;
          Vertex* pVtx = &m_vertices[idxVtx++];
          pVtx->cellIndex[0] = i;
          pVtx->cellIndex[1] = j;
          pVtx->cellIndex[2] = k;
        }
      }
    }
  }
}

// The caller is responsible for bounds checking to allow for optimal performance.
void MMCellMap::getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2])
{
  Cell* pCell = getCell(cellIndex);
  Cell* pCellFirstLabel;
  Cell* pCellSecondLabel;
  switch(edge)
  {
  case MMCellFlag::Edge::LeftBottomEdge:
    pCellFirstLabel = pCell;
    pCellSecondLabel = pCell + m_arraySize[0];
    break;
  case MMCellFlag::Edge::RightBottomEdge:
    pCellFirstLabel = pCell + 1;
    pCellSecondLabel = pCell + 1 + m_arraySize[0];
    break;
  case MMCellFlag::Edge::BackBottomEdge:
    pCellFirstLabel = pCell;
    pCellSecondLabel = pCell + 1;
    break;
  case MMCellFlag::Edge::FrontBottomEdge:
    pCellFirstLabel = pCell + m_arraySize[0];
    pCellSecondLabel = pCell + 1 + m_arraySize[0];
    break;
  case MMCellFlag::Edge::LeftTopEdge:
    pCellFirstLabel = pCell + m_arraySize[0] * m_arraySize[1];
    pCellSecondLabel = pCell + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::RightTopEdge:
    pCellFirstLabel = pCell + 1 + m_arraySize[0] * m_arraySize[1];
    pCellSecondLabel = pCell + 1 + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::BackTopEdge:
    pCellFirstLabel = pCell + m_arraySize[0] * m_arraySize[1];
    pCellSecondLabel = pCell + 1 + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::FrontTopEdge:
    pCellFirstLabel = pCell + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    pCellSecondLabel = pCell + 1 + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::LeftBackEdge:
    pCellFirstLabel = pCell;
    pCellSecondLabel = pCell + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::RightBackEdge:
    pCellFirstLabel = pCell + 1;
    pCellSecondLabel = pCell + 1 + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::LeftFrontEdge:
    pCellFirstLabel = pCell + m_arraySize[0];
    pCellSecondLabel = pCell + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    break;
  case MMCellFlag::Edge::RightFrontEdge:
    pCellFirstLabel = pCell + 1 + m_arraySize[0];
    pCellSecondLabel = pCell + 1 + m_arraySize[0] + m_arraySize[0] * m_arraySize[1];
    break;
  default:
    pCellFirstLabel = pCell;
    pCellSecondLabel = pCell;
    break;
  }
  quadLabels[0] = pCellFirstLabel->label;
  quadLabels[1] = pCellSecondLabel->label;
}

// The caller is responsible for bounds checking to allow for optimal performance.
// Vertices are ordered clockwise around each edge begining with the cell vertex, with
// edges oriented left-to-right, back-to-front and bottom-to-top.
void MMCellMap::getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12])
{
  int vtxIndices[4] = {0, 0, 0, 0};
  getEdgeQuadVtxIndices(cellIndex, edge, vtxIndices);
  for(int i = 0; i < 4; i++)
  {
    int cellIndexTemp[3];
    getVertexCellIndex(vtxIndices[i], cellIndexTemp);
    getVertexPosition(cellIndexTemp, &(quadCorners[i * 3]));
  }
}
// The caller is responsible for bounds checking to allow for optimal performance.
// Vertices are ordered clockwise around each edge begining with the cell vertex, with
// edges oriented left-to-right, back-to-front and bottom-to-top.
void MMCellMap::getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, int quadVtxIndices[4])
{
  Cell* pCell = getCell(cellIndex);
  int length = m_arraySize[0];
  int area = m_arraySize[0] * m_arraySize[1];
  quadVtxIndices[0] = pCell->vertexIndex;
  switch(edge)
  {
  case MMCellFlag::Edge::LeftBottomEdge:
    quadVtxIndices[1] = (pCell - area)->vertexIndex;
    quadVtxIndices[2] = (pCell - 1 - area)->vertexIndex;
    quadVtxIndices[3] = (pCell - 1)->vertexIndex;
    break;
  case MMCellFlag::Edge::RightBottomEdge:
    quadVtxIndices[1] = (pCell + 1)->vertexIndex;
    quadVtxIndices[2] = (pCell + 1 - area)->vertexIndex;
    quadVtxIndices[3] = (pCell - area)->vertexIndex;
    break;
  case MMCellFlag::Edge::BackBottomEdge:
    quadVtxIndices[1] = (pCell - length)->vertexIndex;
    quadVtxIndices[2] = (pCell - length - area)->vertexIndex;
    quadVtxIndices[3] = (pCell - area)->vertexIndex;
    break;
  case MMCellFlag::Edge::FrontBottomEdge:
    quadVtxIndices[1] = (pCell - area)->vertexIndex;
    quadVtxIndices[2] = (pCell + length - area)->vertexIndex;
    quadVtxIndices[3] = (pCell + length)->vertexIndex;
    break;
  case MMCellFlag::Edge::LeftTopEdge:
    quadVtxIndices[1] = (pCell - 1)->vertexIndex;
    quadVtxIndices[2] = (pCell - 1 + area)->vertexIndex;
    quadVtxIndices[3] = (pCell + area)->vertexIndex;
    break;
  case MMCellFlag::Edge::RightTopEdge:
    quadVtxIndices[1] = (pCell + area)->vertexIndex;
    quadVtxIndices[2] = (pCell + 1 + area)->vertexIndex;
    quadVtxIndices[3] = (pCell + 1)->vertexIndex;
    break;
  case MMCellFlag::Edge::BackTopEdge:
    quadVtxIndices[1] = (pCell + area)->vertexIndex;
    quadVtxIndices[2] = (pCell - length + area)->vertexIndex;
    quadVtxIndices[3] = (pCell - length)->vertexIndex;
    break;
  case MMCellFlag::Edge::FrontTopEdge:
    quadVtxIndices[1] = (pCell + length)->vertexIndex;
    quadVtxIndices[2] = (pCell + length + area)->vertexIndex;
    quadVtxIndices[3] = (pCell + area)->vertexIndex;
    break;
  case MMCellFlag::Edge::LeftBackEdge:
    quadVtxIndices[1] = (pCell - 1)->vertexIndex;
    quadVtxIndices[2] = (pCell - 1 - length)->vertexIndex;
    quadVtxIndices[3] = (pCell - length)->vertexIndex;
    break;
  case MMCellFlag::Edge::RightBackEdge:
    quadVtxIndices[1] = (pCell - length)->vertexIndex;
    quadVtxIndices[2] = (pCell + 1 - length)->vertexIndex;
    quadVtxIndices[3] = (pCell + 1)->vertexIndex;
    break;
  case MMCellFlag::Edge::LeftFrontEdge:
    quadVtxIndices[1] = (pCell + length)->vertexIndex;
    quadVtxIndices[2] = (pCell - 1 + length)->vertexIndex;
    quadVtxIndices[3] = (pCell - 1)->vertexIndex;
    break;
  case MMCellFlag::Edge::RightFrontEdge:
    quadVtxIndices[1] = (pCell + 1)->vertexIndex;
    quadVtxIndices[2] = (pCell + 1 + length)->vertexIndex;
    quadVtxIndices[3] = (pCell + length)->vertexIndex;
    break;
  default:
    quadVtxIndices[1] = (pCell)->vertexIndex;
    quadVtxIndices[2] = (pCell)->vertexIndex;
    quadVtxIndices[3] = (pCell)->vertexIndex;
    break;
  }
}

// Access cell map. The caller is responsible for bounds checking.
MMCellMap::Cell* MMCellMap::getCell(int cellIndex[3])
{
  return (&(m_cellArray[cellArrayIndex(cellIndex)]));
}
MMCellMap::Cell* MMCellMap::getCell(int i, int j, int k)
{
  return (&(m_cellArray[cellArrayIndex(i, j, k)]));
}
MMCellMap::Cell* MMCellMap::getCell(int cellMapIndex)
{
  return (&(m_cellArray[cellMapIndex]));
}
int MMCellMap::cellArrayIndex(int cellIndex[3])
{
  return (cellArrayIndex(cellIndex[0], cellIndex[1], cellIndex[2]));
}
int MMCellMap::cellArrayIndex(int i, int j, int k)
{
  return (i + m_arraySize[0] * j + m_arraySize[0] * m_arraySize[1] * k);
}
void MMCellMap::getCellLabels(Cell* pCell, int32_t labels[8])
{
  // Labels of cell's 8 corner vertices. This ordering is used when computing cell
  // flags.
  labels[0] = pCell->label;
  labels[1] = (pCell + 1)->label;
  labels[2] = (pCell + 1 + m_arraySize[0])->label;
  labels[3] = (pCell + m_arraySize[0])->label;
  labels[4] = (pCell + m_arraySize[0] * m_arraySize[1])->label;
  labels[5] = (pCell + 1 + m_arraySize[0] * m_arraySize[1])->label;
  labels[6] = (pCell + 1 + m_arraySize[0] + m_arraySize[0] * m_arraySize[1])->label;
  labels[7] = (pCell + m_arraySize[0] + m_arraySize[0] * m_arraySize[1])->label;
}
bool MMCellMap::isEdgeCrossing(int cellMapIndex, MMCellFlag::Edge edge)
{
  Cell* pCell = getCell(cellMapIndex);
  return (pCell->flag.isEdgeCrossing(edge));
}
MMCellFlag::VertexType MMCellMap::cellVertexType(int cellMapIndex)
{
  Cell* pCell = getCell(cellMapIndex);
  return (pCell->flag.vertexType());
}

// Access vertex data
void MMCellMap::getVertexCellIndex(int vertexIndex, int cellIndex[3])
{
  Vertex* pVertex = &(m_vertices[vertexIndex]);
  cellIndex[0] = pVertex->cellIndex[0];
  cellIndex[1] = pVertex->cellIndex[1];
  cellIndex[2] = pVertex->cellIndex[2];
}
void MMCellMap::getVertexPosition(int cellIndex[3], float position[3])
{
  Cell* pCell = getCell(cellArrayIndex(cellIndex));
  position[0] = m_voxelSize[0] * (cellIndex[0] + pCell->vertexOffset[0]);
  position[1] = m_voxelSize[1] * (cellIndex[1] + pCell->vertexOffset[1]);
  position[2] = m_voxelSize[2] * (cellIndex[2] + pCell->vertexOffset[2]);
}
void MMCellMap::getVertexPosition(int i, int j, int k, float position[3])
{
  Cell* pCell = getCell(i, j, k);
  position[0] = m_voxelSize[0] * (i + pCell->vertexOffset[0]);
  position[1] = m_voxelSize[1] * (j + pCell->vertexOffset[1]);
  position[2] = m_voxelSize[2] * (k + pCell->vertexOffset[2]);
}
int MMCellMap::vertexFaceNeighborVertexIndex(int vertexIndex, MMCellFlag::Face face)
{
  int cellMapIndex(cellArrayIndex(m_vertices[vertexIndex].cellIndex));
  switch(face)
  {
  case MMCellFlag::Face::LeftFace:
    return (cellMapIndex - 1);
  case MMCellFlag::Face::RightFace:
    return (cellMapIndex + 1);
  case MMCellFlag::Face::BackFace:
    return (cellMapIndex - m_arraySize[0]);
  case MMCellFlag::Face::FrontFace:
    return (cellMapIndex + m_arraySize[0]);
  case MMCellFlag::Face::BottomFace:
    return (cellMapIndex - m_arraySize[0] * m_arraySize[1]);
  case MMCellFlag::Face::TopFace:
    return (cellMapIndex + m_arraySize[0] * m_arraySize[1]);
  default:
    return (cellMapIndex);
  }
}

// Access cell neighbors
MMCellMap::Cell* MMCellMap::getFaceNeighborCellAndIndex(int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3])
{
  nbrCellIndex[0] = cellIndex[0];
  nbrCellIndex[1] = cellIndex[1];
  nbrCellIndex[2] = cellIndex[2];
  switch(face)
  {
  case MMCellFlag::Face::LeftFace:
    nbrCellIndex[0] -= 1;
    break;
  case MMCellFlag::Face::RightFace:
    nbrCellIndex[0] += 1;
    break;
  case MMCellFlag::Face::BackFace:
    nbrCellIndex[1] -= 1;
    break;
  case MMCellFlag::Face::FrontFace:
    nbrCellIndex[1] += 1;
    break;
  case MMCellFlag::Face::BottomFace:
    nbrCellIndex[2] -= 1;
    break;
  case MMCellFlag::Face::TopFace:
    nbrCellIndex[2] += 1;
    break;
  default:
    break;
  }
  return (&(m_cellArray[cellArrayIndex(nbrCellIndex)]));
}

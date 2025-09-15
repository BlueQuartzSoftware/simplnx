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

namespace
{

void initCell(MMCellMap::Cell& cell, int32_t label)
{
  cell.label = label;
  cell.flag.clear();
  cell.vertexIndex = -1;
  // cell.vertexOffset[0] = 0.5f;
  // cell.vertexOffset[1] = 0.5f;
  // cell.vertexOffset[2] = 0.5f;
}

} // namespace

// Basic cell map containing material labels
MMCellMap::MMCellMap(TriangleGeom& triangleGeometry, int arraySize[3], float voxelSize[3])
: m_cellArray(nullptr)
, m_numVertices(0)
{
  m_NxDims = {arraySize[0], arraySize[1], arraySize[2]};
  // Allocate memory for the cell map. To ensure closed shapes and sharp corners
  // and edges at volume faces, faces are padded by one voxel with a reserved
  // label.
  for(int i = 0; i < 3; i++)
  {
    m_arraySize[i] = arraySize[i] + 2;
    m_voxelSize[i] = voxelSize[i];
  }
  size_t numCells = m_arraySize[0] * m_arraySize[1] * m_arraySize[2];
  try
  {
    m_cellArray = new Cell[numCells];
  } catch(std::bad_alloc& ba)
  {
    m_cellArray = nullptr;
    return;
  }

  m_VertexListPtr = triangleGeometry.getVertices();
}

bool MMCellMap::valid() const
{
  return m_cellArray != nullptr;
}

bool MMCellMap::init(Int32Array& labels)
{
  // Initialize interior cell contents. Each cell stores the label of it's bottom-left back corner.
  usize cellIndex = 0;
  usize labelIndex = 0;
  for(int k = 0; k < m_arraySize[2]; k++)
  {
    for(int j = 0; j < m_arraySize[1]; j++)
    {
      for(int i = 0; i < m_arraySize[0]; i++)
      {
        if(i == 0 || i == m_arraySize[0] - 1 || j == 0 || j == m_arraySize[1] - 1 || k == 0 || k == m_arraySize[2] - 1)
        {
          initCell(m_cellArray[cellIndex++], static_cast<int32_t>(MMSurfaceNet::ReservedLabel::Padding));
        }
        else
        {
          initCell(m_cellArray[cellIndex++], labels[labelIndex++]);
        }
      }
    }
  }

  // Set the cell vertices
  return setCellVertices();
}

MMCellMap::~MMCellMap()
{
  delete[] m_cellArray;
  // delete[] m_vertices;
}

// Relax vertex positions using relaxation attributes or reset to cell centers
void MMCellMap::relax(const MMSurfaceNet::RelaxAttrs& relaxAttrs)
{
  for(int i = 0; i < relaxAttrs.numRelaxIterations; i++)
  {
    for(size_t idxVtx = 0; idxVtx < m_numVertices; idxVtx++)
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
            const Vertex* nbrVertex = &(m_vertices[nbrCell->vertexIndex]);

            avgP[0] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3) + static_cast<float>(nbrIdx[0] - cellIdx[0]);
            avgP[1] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3 + 1) + static_cast<float>(nbrIdx[1] - cellIdx[1]);
            avgP[2] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3 + 2) + static_cast<float>(nbrIdx[2] - cellIdx[2]);
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
            const Vertex* nbrVertex = &(m_vertices[nbrCell->vertexIndex]);

            avgP[0] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3) + static_cast<float>(nbrIdx[0] - cellIdx[0]);
            avgP[1] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3 + 1) + static_cast<float>(nbrIdx[1] - cellIdx[1]);
            avgP[2] += m_VertexListPtr->getValue(nbrCell->vertexIndex * 3 + 2) + static_cast<float>(nbrIdx[2] - cellIdx[2]);
            numNeighbors++;
          }
        }
      }

      // Add a fraction of the averaged vertex position to the current position
      if(numNeighbors > 0)
      {
        // Constrain vertex location to a max distance from the original voxel
        const float min = 0.5 - relaxAttrs.maxDistFromCellCenter;
        const float max = 0.5 + relaxAttrs.maxDistFromCellCenter;

        Vertex* pVertex = &(m_vertices[pCell->vertexIndex]);

        avgP[0] /= static_cast<float>(numNeighbors);
        avgP[1] /= static_cast<float>(numNeighbors);
        avgP[2] /= static_cast<float>(numNeighbors);
        const float alpha = relaxAttrs.relaxFactor;
        float x = (1.0f - alpha) * m_VertexListPtr->getValue(pCell->vertexIndex * 3) + alpha * avgP[0];
        if(x < min)
        {
          x = min;
        }
        if(x > max)
        {
          x = max;
        }
        m_VertexListPtr->setValue(pCell->vertexIndex * 3, x);

        float y = (1.0f - alpha) * m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 1) + alpha * avgP[1];
        if(y < min)
        {
          y = min;
        }
        if(y > max)
        {
          y = max;
        }
        m_VertexListPtr->setValue(pCell->vertexIndex * 3 + 1, y);

        float z = (1.0f - alpha) * m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 2) + alpha * avgP[2];
        if(z < min)
        {
          z = min;
        }
        if(z > max)
        {
          z = max;
        }
        m_VertexListPtr->setValue(pCell->vertexIndex * 3 + 2, z);
      }
    }
  }
}
void MMCellMap::reset() const
{
  for(size_t idxVtx = 0; idxVtx < m_numVertices; idxVtx++)
  {
    int cellIdx[3];
    getVertexCellIndex(idxVtx, cellIdx);
    Cell* pCell = getCell(cellIdx);
  }
}

// Data for export
void MMCellMap::getArraySize(int arraySize[3]) const
{
  arraySize[0] = m_arraySize[0];
  arraySize[1] = m_arraySize[1];
  arraySize[2] = m_arraySize[2];
}
void MMCellMap::getVoxelSize(float voxelSize[3]) const
{
  voxelSize[0] = m_voxelSize[0];
  voxelSize[1] = m_voxelSize[1];
  voxelSize[2] = m_voxelSize[2];
}
size_t MMCellMap::numVertices() const
{
  return m_numVertices;
}
size_t MMCellMap::numEdgeCrossings() const
{
  size_t cellMapIdx = 0;
  size_t numCrossings = 0;
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
MMCellFlag::VertexType MMCellMap::vertexType(size_t vertexIndex) const
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
bool MMCellMap::getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, float quadCorners[12], int32_t quadLabels[2], usize quadNxArrayIndices[2])
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  if(!isEdgeCrossing(cellArrayIndex(cellIndex), edge))
  {
    return false;
  }

  // Because there is an edge crossing, cell map access in the following will be
  // in-bounds by construction of the cell map.
  getEdgeLabels(cellIndex, edge, quadLabels, quadNxArrayIndices);
  getEdgeQuadPositions(cellIndex, edge, quadCorners);
  return true;
}
// Returns true if there is an edge crossing and false otherwise. If there is an edge
// crossing, we defince a surface quad from vertices in the 4 cells touching the edge.
// The indices of these 4 vertices are inserted into quadVtxIndices in clockwise order
// and the quad face labels are inserted into quadLabels as [labelTopFaceOfQuad,
// labelBottomFaceOfQuad]. If there is no edge crossing, quadCorners and quadLabels
// will not be set.
bool MMCellMap::getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, size_t quadVtxIndices[4], int32_t quadLabels[2], usize quadNxArrayIndices[2])
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  if(!isEdgeCrossing(cellArrayIndex(cellIndex), edge))
  {
    return false;
  }

  // Because there is an edge crossing, cell map access in the following will be
  // in-bounds by construction of the cell map.
  getEdgeLabels(cellIndex, edge, quadLabels, quadNxArrayIndices);
  getEdgeQuadVtxIndices(cellIndex, edge, quadVtxIndices);
  return true;
}

void MMCellMap::getVertexPosition(size_t vertexIndex, float position[3]) const
{
  getVertexPosition(m_vertices[vertexIndex].cellIndex, position);
}

bool MMCellMap::setCellVertices()
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
    //delete[] m_vertices;
    m_VertexListPtr->resizeTuples({m_numVertices});
    m_vertices = new Vertex[m_numVertices];
  } catch(std::bad_alloc& ba)
  {
    delete[] m_cellArray;
    m_cellArray = nullptr;
    m_numVertices = 0;
    m_vertices = nullptr;
    return false;
  }
  auto& vertexDataStoreRef = m_VertexListPtr->getDataStoreRef();
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
          m_vertices[idxVtx].cellIndex[0] = i;
          m_vertices[idxVtx].cellIndex[1] = j;
          m_vertices[idxVtx].cellIndex[2] = k;

          vertexDataStoreRef.setValue(idxVtx * 3, 0.5f);
          vertexDataStoreRef.setValue(idxVtx * 3 + 1, 0.5f);
          vertexDataStoreRef.setValue(idxVtx * 3 + 2, 0.5f);

          idxVtx++;
        }
      }
    }
  }
  return true;
}

// The caller is responsible for bounds checking to allow for optimal performance.
void MMCellMap::getEdgeLabels(int cellIndex[3], MMCellFlag::Edge edge, int32_t quadLabels[2], usize quadNxArrayIndices[2])
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

  quadNxArrayIndices[0] = getNxCellArrayIndex(pCellFirstLabel->vertexIndex);
  quadNxArrayIndices[1] = getNxCellArrayIndex(pCellSecondLabel->vertexIndex);

  quadLabels[0] = pCellFirstLabel->label;
  quadLabels[1] = pCellSecondLabel->label;
}

usize MMCellMap::getNxCellArrayIndex(int64_t vertexIndex)
{
  usize nxArrayIdx = std::numeric_limits<usize>::max();
  if(vertexIndex < 0)
  {
    return nxArrayIdx;
  }
  std::array<int, 3> cellIndex = {0, 0, 0};
  getVertexCellIndex(vertexIndex, cellIndex.data());
  if(cellIndex[0] - 1 >= 0 && cellIndex[0] - 1 < m_arraySize[0] - 1 && cellIndex[1] - 1 > 0 && cellIndex[1] - 1 < m_arraySize[1] - 1 && cellIndex[2] - 1 > 0 && cellIndex[2] - 1 < m_arraySize[2] - 1)
  {
    nxArrayIdx = ((cellIndex[2] - 1) * m_NxDims[1] * m_NxDims[0]) + ((cellIndex[1] - 1) * m_NxDims[0]) + (cellIndex[0] - 1);
  }
  return nxArrayIdx;
}

// The caller is responsible for bounds checking to allow for optimal performance.
// Vertices are ordered clockwise around each edge beginning with the cell vertex, with
// edges oriented left-to-right, back-to-front and bottom-to-top.
void MMCellMap::getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12])
{
  size_t vtxIndices[4] = {0, 0, 0, 0};
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
void MMCellMap::getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const
{
  Cell* pCell = getCell(cellIndex);
  const int length = m_arraySize[0];
  const int area = m_arraySize[0] * m_arraySize[1];
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
MMCellMap::Cell* MMCellMap::getCell(int cellIndex[3]) const
{
  return (&(m_cellArray[cellArrayIndex(cellIndex)]));
}
MMCellMap::Cell* MMCellMap::getCell(int i, int j, int k) const
{
  return (&(m_cellArray[cellArrayIndex(i, j, k)]));
}
MMCellMap::Cell* MMCellMap::getCell(size_t cellMapIndex) const
{
  return (&(m_cellArray[cellMapIndex]));
}
size_t MMCellMap::cellArrayIndex(int cellIndex[3]) const
{
  return (cellArrayIndex(cellIndex[0], cellIndex[1], cellIndex[2]));
}
size_t MMCellMap::cellArrayIndex(int i, int j, int k) const
{
  const auto ii = static_cast<size_t>(i);
  const auto jj = static_cast<size_t>(j);
  const auto kk = static_cast<size_t>(k);
  return (ii + m_arraySize[0] * jj + m_arraySize[0] * m_arraySize[1] * kk);
}
void MMCellMap::getCellLabels(Cell* pCell, int32_t labels[8]) const
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
bool MMCellMap::isEdgeCrossing(size_t cellMapIndex, MMCellFlag::Edge edge) const
{
  Cell* pCell = getCell(cellMapIndex);
  return (pCell->flag.isEdgeCrossing(edge));
}
MMCellFlag::VertexType MMCellMap::cellVertexType(size_t cellMapIndex) const
{
  Cell* pCell = getCell(cellMapIndex);
  return (pCell->flag.vertexType());
}

// Access vertex data
void MMCellMap::getVertexCellIndex(size_t vertexIndex, int cellIndex[3]) const
{
  const Vertex* pVertex = &(m_vertices[vertexIndex]);
  cellIndex[0] = pVertex->cellIndex[0];
  cellIndex[1] = pVertex->cellIndex[1];
  cellIndex[2] = pVertex->cellIndex[2];
}
void MMCellMap::getVertexPosition(int cellIndex[3], float position[3]) const
{
  Cell* pCell = getCell(cellArrayIndex(cellIndex));
  position[0] = m_voxelSize[0] * (cellIndex[0] + m_VertexListPtr->getValue(pCell->vertexIndex * 3));
  position[1] = m_voxelSize[1] * (cellIndex[1] + m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 1));
  position[2] = m_voxelSize[2] * (cellIndex[2] + m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 2));
}
void MMCellMap::getVertexPosition(int i, int j, int k, float position[3]) const
{
  Cell* pCell = getCell(i, j, k);
  position[0] = m_voxelSize[0] * (i + m_VertexListPtr->getValue(pCell->vertexIndex * 3));
  position[1] = m_voxelSize[1] * (j + m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 1));
  position[2] = m_voxelSize[2] * (k + m_VertexListPtr->getValue(pCell->vertexIndex * 3 + 2));
}
int MMCellMap::vertexFaceNeighborVertexIndex(size_t vertexIndex, MMCellFlag::Face face) const
{
  size_t cellMapIndex(cellArrayIndex(m_vertices[vertexIndex].cellIndex));
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

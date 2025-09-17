// MMCellMap.cpp
//
// MMCellMap implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "MMCellMap.h"
#include "MMSurfaceNet.h"

namespace
{

/**
 * @brief Converts from an IJK Surface Nets Volume index to an NX Volume IJK index
 * @param cellIndex SurfaceNets Cell Index
 * @param nxCellIndex Output NX Cell Index
 */
void ToNxIJK(const int32* cellIndex, int32* nxCellIndex)
{
  nxCellIndex[0] = cellIndex[0] - 1;
  nxCellIndex[1] = cellIndex[1] - 1;
  nxCellIndex[2] = cellIndex[2] - 1;
}

/**
 *
 * @param cellIndex This is the index of the cell in SurfaceNets Volume IJK indices
 * @param nxDims
 * @return
 */
size_t ToNxFlatIndex(const int32* cellIndex, const size_t* nxDims)
{
  std::array<int32_t, 3> nxCellIndex = {0, 0, 0};
  ToNxIJK(cellIndex, nxCellIndex.data());
  return (static_cast<size_t>(nxCellIndex[2]) * nxDims[1] * nxDims[0]) + (static_cast<size_t>(nxCellIndex[1]) * nxDims[0]) + (static_cast<size_t>(nxCellIndex[0]));
}

/**
 *
 * @param cellIndex SurfaceNets Cell Index
 * @param nxDims The dimensions of the NX Volume
 * @return
 */
bool InsideNxVolume(const int32* cellIndex, const std::array<size_t, 3>& nxDims)
{
  std::array<int32_t, 3> nxCellIndex = {0, 0, 0};
  ToNxIJK(cellIndex, nxCellIndex.data());

  for(int i = 0; i < 3; ++i)
  {
    if(nxCellIndex[i] < 0 || nxCellIndex[i] >= nxDims[i])
    {
      return false;
    }
  }
  return true;
}

} // namespace

// ----------------------------------------------------------------------------
MMCellMap::MMCellMap(TriangleGeom::SharedVertexList::store_type& verticesStore, Int32Array* labels, size_t arraySize[3], const float voxelSize[3])
: m_cellArray(nullptr)
, m_VerticesStoreRef(verticesStore)
, m_NxLabelsPtr(labels)
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
}

// ----------------------------------------------------------------------------
MMCellMap::~MMCellMap()
{
  delete[] m_cellArray;
}

// ----------------------------------------------------------------------------
bool MMCellMap::valid() const
{
  return m_cellArray != nullptr;
}

// ----------------------------------------------------------------------------
bool MMCellMap::init()
{
  // Initialize interior cell contents. Each cell stores the label of it's bottom-left back corner.
  usize cellIndex = 0;

  for(int k = 0; k < m_arraySize[2]; k++)
  {
    for(int j = 0; j < m_arraySize[1]; j++)
    {
      for(int i = 0; i < m_arraySize[0]; i++)
      {
        if(i == 0 || i == m_arraySize[0] - 1 || j == 0 || j == m_arraySize[1] - 1 || k == 0 || k == m_arraySize[2] - 1)
        {
          Cell& cell = m_cellArray[cellIndex++];
          cell.flag.clear();
          cell.vertexIndex = std::numeric_limits<size_t>::max();
        }
        else
        {
          Cell& cell = m_cellArray[cellIndex++];
          cell.flag.clear();
          cell.vertexIndex = std::numeric_limits<size_t>::max();
        }
      }
    }
  }

  // Set the cell vertices
  return setCellVertices();
}

// ----------------------------------------------------------------------------
bool MMCellMap::setCellVertices()
{
  // Set cell type and count cell vertices. There are no vertices in right, front,
  // top faces.
  size_t numVertices = 0;
  int32 cellIndex[3] = {0, 0, 0};
  for(int k = 0; k < m_arraySize[2] - 1; k++)
  {
    for(int j = 0; j < m_arraySize[1] - 1; j++)
    {
      for(int i = 0; i < m_arraySize[0] - 1; i++)
      {
        Cell* pCell = getCell(i, j, k);
        int32_t cellLabels[8];

        cellIndex[0] = i;
        cellIndex[1] = j;
        cellIndex[2] = k;
        // Label[0]
        cellLabels[0] = label(cellIndex);

        // Label[1]
        cellIndex[0] = i + 1;
        cellLabels[1] = label(cellIndex);

        // Label[2]
        cellIndex[0] = i + 1;
        cellIndex[1] = j + 1;
        cellLabels[2] = label(cellIndex);

        // Label[3]
        cellIndex[0] = i;
        cellIndex[1] = j + 1;
        cellLabels[3] = label(cellIndex);

        // Label[4]
        cellIndex[0] = i;
        cellIndex[1] = j;
        cellIndex[2] = k + 1;
        cellLabels[4] = label(cellIndex);

        // Label[5]
        cellIndex[0] = i + 1;
        cellIndex[1] = j;
        cellIndex[2] = k + 1;
        cellLabels[5] = label(cellIndex);

        // Label[6]
        cellIndex[0] = i + 1;
        cellIndex[1] = j + 1;
        cellIndex[2] = k + 1;
        cellLabels[6] = label(cellIndex);

        // Label[7]
        cellIndex[0] = i;
        cellIndex[1] = j + 1;
        cellIndex[2] = k + 1;
        cellLabels[7] = label(cellIndex);

        pCell->flag.set(cellLabels);
        if(pCell->flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          numVertices++;
        }
      }
    }
  }

  // Create cell vertices. There are no vertices in right, front, top faces.
  try
  {
    m_VerticesStoreRef.resizeTuples(IDataStore::ShapeType{numVertices});
    m_VertexArray.resize(numVertices);
  } catch(std::bad_alloc& ba)
  {
    delete[] m_cellArray;
    m_cellArray = nullptr;
    return false;
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
          m_VertexArray[idxVtx].cellIndex[0] = i;
          m_VertexArray[idxVtx].cellIndex[1] = j;
          m_VertexArray[idxVtx].cellIndex[2] = k;

          m_VerticesStoreRef.setValue(idxVtx * 3, 0.5f);
          m_VerticesStoreRef.setValue(idxVtx * 3 + 1, 0.5f);
          m_VerticesStoreRef.setValue(idxVtx * 3 + 2, 0.5f);

          idxVtx++;
        }
      }
    }
  }
  return true;
}

int32_t MMCellMap::label(const int32 cellIndex[3]) const
{
  if(cellIndex[0] == 0 || cellIndex[1] == 0 || cellIndex[2] == 0)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }
  if(cellIndex[0] >= m_arraySize[0] - 1 || cellIndex[1] >= m_arraySize[1] - 1 || cellIndex[2] >= m_arraySize[2] - 1)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }

  int32_t label = 0x8BABABAB;
  if(cellIndex[0] - 1 >= 0 && cellIndex[0] - 1 < m_arraySize[0] - 1 && cellIndex[1] - 1 >= 0 && cellIndex[1] - 1 < m_arraySize[1] - 1 && cellIndex[2] - 1 >= 0 && cellIndex[2] - 1 < m_arraySize[2] - 1)
  {
    size_t nxArrayIdx = ToNxFlatIndex(cellIndex, m_NxDims.data());
    label = m_NxLabelsPtr->at(nxArrayIdx);
  }
  return label;
}

// Relax vertex positions using relaxation attributes or reset to cell centers
void MMCellMap::relax(const MMSurfaceNet::RelaxAttrs& relaxAttrs) const
{
  size_t numVertices = m_VertexArray.size();
  for(int i = 0; i < relaxAttrs.numRelaxIterations; i++)
  {
    for(size_t idxVtx = 0; idxVtx < numVertices; idxVtx++)
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
            float avgpX = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3);
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3 + 1);
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3 + 2);
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);

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

            float avgpX = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3);
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3 + 1);
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrCell->vertexIndex == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCell->vertexIndex * 3 + 2);
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);
            numNeighbors++;
          }
        }
      }

      // Add a fraction of the averaged vertex position to the current position
      if(numNeighbors > 0)
      {
        // Constrain vertex location to a max distance from the original voxel
        const float min = 0.5f - relaxAttrs.maxDistFromCellCenter;
        const float max = 0.5f + relaxAttrs.maxDistFromCellCenter;

        avgP[0] /= static_cast<float>(numNeighbors);
        avgP[1] /= static_cast<float>(numNeighbors);
        avgP[2] /= static_cast<float>(numNeighbors);
        const float alpha = relaxAttrs.relaxFactor;

        float x = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCell->vertexIndex * 3) + alpha * avgP[0];
        x = std::clamp(x, min, max);
        m_VerticesStoreRef.setValue(pCell->vertexIndex * 3, x);

        float y = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 1) + alpha * avgP[1];
        y = std::clamp(y, min, max);
        m_VerticesStoreRef.setValue(pCell->vertexIndex * 3 + 1, y);

        float z = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 2) + alpha * avgP[2];
        z = std::clamp(z, min, max);
        m_VerticesStoreRef.setValue(pCell->vertexIndex * 3 + 2, z);
      }
    }
  }
}

// Data for export
void MMCellMap::getArraySize(int arraySize[3]) const
{
  arraySize[0] = static_cast<int32_t>(m_arraySize[0]);
  arraySize[1] = static_cast<int32_t>(m_arraySize[1]);
  arraySize[2] = static_cast<int32_t>(m_arraySize[2]);
}

void MMCellMap::getVoxelSize(float voxelSize[3]) const
{
  voxelSize[0] = m_voxelSize[0];
  voxelSize[1] = m_voxelSize[1];
  voxelSize[2] = m_voxelSize[2];
}

size_t MMCellMap::numVertices() const
{
  return m_VertexArray.size();
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
// crossing, we define a surface quad from vertices in the 4 cells touching the edge.
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
// crossing, we define a surface quad from vertices in the 4 cells touching the edge.
// The indices of these 4 vertices are inserted into quadVtxIndices in clockwise order
// and the quad face labels are inserted into quadLabels as [labelTopFaceOfQuad,
// labelBottomFaceOfQuad]. If there is no edge crossing, quadCorners and quadLabels
// will not be set.
bool MMCellMap::getEdgeQuad(size_t vertexIndex, MMCellFlag::Edge edge, size_t quadVtxIndices[4], int32_t quadLabels[2], usize quadNxArrayIndices[2]) const
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
  const Vertex& vertex = m_VertexArray[vertexIndex];
  const int32_t* cellIndex = vertex.cellIndex;
  getVertexPosition(cellIndex, position);
}

// The caller is responsible for bounds checking to allow for optimal performance.
void MMCellMap::getEdgeLabels(int cellIndex[3], const MMCellFlag::Edge edge, int32_t quadLabels[2], usize quadNxArrayIndices[2]) const
{
  std::array<int32_t, 3> firstCellLabels;
  std::array<int32_t, 3> secondCellLabels;

  switch(edge)
  {
  case MMCellFlag::Edge::LeftBottomEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2]};
    break;
  case MMCellFlag::Edge::RightBottomEdge:
    firstCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2]};
    break;
  case MMCellFlag::Edge::BackBottomEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2]};
    break;
  case MMCellFlag::Edge::FrontBottomEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2]};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2]};
    break;
  case MMCellFlag::Edge::LeftTopEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2] + 1};
    secondCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::RightTopEdge:
    firstCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2] + 1};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::BackTopEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2] + 1};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::FrontTopEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2] + 1};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::LeftBackEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::RightBackEdge:
    firstCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::LeftFrontEdge:
    firstCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2]};
    secondCellLabels = {cellIndex[0], cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  case MMCellFlag::Edge::RightFrontEdge:
    firstCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2]};
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1] + 1, cellIndex[2] + 1};
    break;
  default:
    firstCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2]};
    secondCellLabels = {cellIndex[0], cellIndex[1], cellIndex[2]};
    break;
  }

  quadNxArrayIndices[0] = std::numeric_limits<usize>::max();
  quadNxArrayIndices[1] = std::numeric_limits<usize>::max();

  if(InsideNxVolume(firstCellLabels.data(), m_NxDims))
  {
    quadNxArrayIndices[0] = ToNxFlatIndex(firstCellLabels.data(), m_NxDims.data());
  }

  if(InsideNxVolume(secondCellLabels.data(), m_NxDims))
  {
    quadNxArrayIndices[1] = ToNxFlatIndex(secondCellLabels.data(), m_NxDims.data());
  }

  quadLabels[0] = label(firstCellLabels.data());
  quadLabels[1] = label(secondCellLabels.data());
}

usize MMCellMap::getNxCellArrayIndex(size_t vertexIndex) const
{
  usize nxArrayIdx = 0;
  if(vertexIndex == std::numeric_limits<usize>::max())
  {
    return vertexIndex;
  }
  std::array<int32_t, 3> cellIndex = {0, 0, 0};
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
void MMCellMap::getEdgeQuadPositions(int cellIndex[3], MMCellFlag::Edge edge, float quadCorners[12]) const
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
// Vertices are ordered clockwise around each edge beginning with the cell vertex, with
// edges oriented left-to-right, back-to-front and bottom-to-top.
void MMCellMap::getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const
{
  Cell* pCell = getCell(cellIndex);
  const int length = static_cast<int32_t>(m_arraySize[0]);
  const int area = static_cast<int32_t>(m_arraySize[0] * m_arraySize[1]);
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

size_t MMCellMap::cellArrayIndex(const int cellIndex[3]) const
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
  Cell* cellPtr = pCell;
  usize vertIdx = cellPtr->vertexIndex;
  labels[0] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = pCell + 1;
  vertIdx = cellPtr->vertexIndex;
  labels[1] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + 1 + m_arraySize[0]);
  vertIdx = cellPtr->vertexIndex;
  labels[2] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + m_arraySize[0]);
  vertIdx = cellPtr->vertexIndex;
  labels[3] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + m_arraySize[0] * m_arraySize[1]);
  vertIdx = cellPtr->vertexIndex;
  labels[4] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + 1 + m_arraySize[0] * m_arraySize[1]);
  vertIdx = cellPtr->vertexIndex;
  labels[5] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + 1 + m_arraySize[0] + m_arraySize[0] * m_arraySize[1]);
  vertIdx = cellPtr->vertexIndex;
  labels[6] = label(m_VertexArray.at(vertIdx).cellIndex);

  cellPtr = (pCell + m_arraySize[0] + m_arraySize[0] * m_arraySize[1]);
  vertIdx = cellPtr->vertexIndex;
  labels[7] = label(m_VertexArray.at(vertIdx).cellIndex);
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
  if(vertexIndex == std::numeric_limits<size_t>::max())
  {
    cellIndex[0] = 0;
    cellIndex[1] = 0;
    cellIndex[2] = 0;
    return;
  }
  const Vertex* pVertex = &(m_VertexArray[vertexIndex]);
  cellIndex[0] = pVertex->cellIndex[0];
  cellIndex[1] = pVertex->cellIndex[1];
  cellIndex[2] = pVertex->cellIndex[2];
}

void MMCellMap::getVertexPosition(const int cellIndex[3], float position[3]) const
{
  const Cell* pCell = getCell(cellArrayIndex(cellIndex));

  position[0] = m_voxelSize[0] * (static_cast<float>(cellIndex[0]) + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3));
  position[1] = m_voxelSize[1] * (static_cast<float>(cellIndex[1]) + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 1));
  position[2] = m_voxelSize[2] * (static_cast<float>(cellIndex[2]) + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 2));
}

void MMCellMap::getVertexPosition(int i, int j, int k, float position[3]) const
{
  const Cell* pCell = getCell(i, j, k);

  position[0] = m_voxelSize[0] * (i + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3));
  position[1] = m_voxelSize[1] * (j + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 1));
  position[2] = m_voxelSize[2] * (k + m_VerticesStoreRef.getValue(pCell->vertexIndex * 3 + 2));
}

usize MMCellMap::vertexFaceNeighborVertexIndex(size_t vertexIndex, MMCellFlag::Face face) const
{
  const Vertex& vertex = m_VertexArray[vertexIndex];
  const int32_t* cellIndex = vertex.cellIndex;

  size_t cellMapIndex = cellArrayIndex(cellIndex);
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
MMCellMap::Cell* MMCellMap::getFaceNeighborCellAndIndex(const int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]) const
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

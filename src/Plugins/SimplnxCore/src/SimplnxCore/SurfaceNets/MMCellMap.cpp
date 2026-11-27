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
MMCellMap::MMCellMap(TriangleGeom::SharedVertexListType::StoreType& verticesStore, Int32ArrayType* labels, size_t arraySize[3], const float voxelSize[3])
: m_CellArray(nullptr)
, m_VerticesStoreRef(verticesStore)
, m_NxLabelsPtr(labels)
{
  m_NxDims = {arraySize[0], arraySize[1], arraySize[2]};
  // Allocate memory for the cell map. To ensure closed shapes and sharp corners
  // and edges at volume faces, faces are padded by one voxel with a reserved
  // label.
  for(int i = 0; i < 3; i++)
  {
    m_ArraySize[i] = arraySize[i] + 2;
    m_VoxelSize[i] = voxelSize[i];
  }
  size_t numCells = m_ArraySize[0] * m_ArraySize[1] * m_ArraySize[2];
  try
  {
    m_CellArray = new Cell[numCells];
  } catch(std::bad_alloc& ba)
  {
    m_CellArray = nullptr;
    return;
  }
}

// ----------------------------------------------------------------------------
MMCellMap::~MMCellMap()
{
  delete[] m_CellArray;
}

// ----------------------------------------------------------------------------
bool MMCellMap::valid() const
{
  return m_CellArray != nullptr;
}

// ----------------------------------------------------------------------------
Result<> MMCellMap::init()
{
  // Initialize interior cell contents. Each cell stores the label of it's bottom-left back corner.
  usize cellIndex = 0;

  for(int k = 0; k < m_ArraySize[2]; k++)
  {
    for(int j = 0; j < m_ArraySize[1]; j++)
    {
      for(int i = 0; i < m_ArraySize[0]; i++)
      {
        if(i == 0 || i == m_ArraySize[0] - 1 || j == 0 || j == m_ArraySize[1] - 1 || k == 0 || k == m_ArraySize[2] - 1)
        {
          Cell& cell = m_CellArray[cellIndex++];
          cell.flag.clear();
          cell.vertex_index = std::numeric_limits<size_t>::max();
        }
        else
        {
          Cell& cell = m_CellArray[cellIndex++];
          cell.flag.clear();
          cell.vertex_index = std::numeric_limits<size_t>::max();
        }
      }
    }
  }

  // Set the cell vertices
  return setCellVertices();
}

// ----------------------------------------------------------------------------
Result<> MMCellMap::setCellVertices()
{
  // Set cell type and count cell vertices. There are no vertices in right, front,
  // top faces.
  size_t numVertices = 0;
  for(int k = 0; k < m_ArraySize[2] - 1; k++)
  {
    for(int j = 0; j < m_ArraySize[1] - 1; j++)
    {
      for(int i = 0; i < m_ArraySize[0] - 1; i++)
      {
        Cell* pCellPtr = getCell(i, j, k);
        int32_t cellLabels[8];
        const int cornerCellIndex[3] = {i, j, k};
        getCornerLabels(cornerCellIndex, cellLabels);
        pCellPtr->flag.set(cellLabels);
        if(pCellPtr->flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          numVertices++;
        }
      }
    }
  }

  // Create cell vertices. There are no vertices in right, front, top faces.
  try
  {
    Result<> resizeResult = m_VerticesStoreRef.resizeTuples(ShapeType{numVertices});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
    m_VertexArray.resize(numVertices);
  } catch(std::bad_alloc& ba)
  {
    delete[] m_CellArray;
    m_CellArray = nullptr;
    return MakeErrorResult(-843871, "SurfaceNets could not allocate its cell-vertex index.");
  }
  int idxVtx = 0;
  for(int k = 0; k < m_ArraySize[2] - 1; k++)
  {
    for(int j = 0; j < m_ArraySize[1] - 1; j++)
    {
      for(int i = 0; i < m_ArraySize[0] - 1; i++)
      {
        Cell* pCellPtr = getCell(i, j, k);
        if(pCellPtr->flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          pCellPtr->vertex_index = idxVtx;
          m_VertexArray[idxVtx].cell_index[0] = i;
          m_VertexArray[idxVtx].cell_index[1] = j;
          m_VertexArray[idxVtx].cell_index[2] = k;

          m_VerticesStoreRef.setValue(idxVtx * 3, 0.5f);
          m_VerticesStoreRef.setValue(idxVtx * 3 + 1, 0.5f);
          m_VerticesStoreRef.setValue(idxVtx * 3 + 2, 0.5f);

          idxVtx++;
        }
      }
    }
  }
  return {};
}

int32_t MMCellMap::label(const int32 cellIndex[3]) const
{
  if(cellIndex[0] == 0 || cellIndex[1] == 0 || cellIndex[2] == 0)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }
  if(cellIndex[0] >= m_ArraySize[0] - 1 || cellIndex[1] >= m_ArraySize[1] - 1 || cellIndex[2] >= m_ArraySize[2] - 1)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }

  int32_t label = 0x8BABABAB;
  if(cellIndex[0] - 1 >= 0 && cellIndex[0] - 1 < m_ArraySize[0] - 1 && cellIndex[1] - 1 >= 0 && cellIndex[1] - 1 < m_ArraySize[1] - 1 && cellIndex[2] - 1 >= 0 && cellIndex[2] - 1 < m_ArraySize[2] - 1)
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
  for(int i = 0; i < relaxAttrs.num_relax_iterations; i++)
  {
    for(size_t idxVtx = 0; idxVtx < numVertices; idxVtx++)
    {
      int cellIdx[3];
      getVertexCellIndex(idxVtx, cellIdx);
      Cell* pCellPtr = getCell(cellIdx);

      int numNeighbors = 0;
      float avgP[3] = {0.0f, 0.0f, 0.0f};
      if(pCellPtr->flag.vertexType() == MMCellFlag::VertexType::SurfaceVertex)
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(pCellPtr->flag.faceCrossingType(face) != MMCellFlag::FaceCrossingType::NoFaceCrossing)
          {
            int nbrIdx[3];
            Cell* nbrCellPtr = getFaceNeighborCellAndIndex(cellIdx, face, nbrIdx);
            float avgpX = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3);
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3 + 1);
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3 + 2);
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);

            numNeighbors++;
          }
        }
      }
      else
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(pCellPtr->flag.faceCrossingType(face) == MMCellFlag::FaceCrossingType::JunctionFaceCrossing)
          {
            int nbrIdx[3];
            Cell* nbrCellPtr = getFaceNeighborCellAndIndex(cellIdx, face, nbrIdx);

            float avgpX = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3);
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3 + 1);
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrCellPtr->vertex_index == std::numeric_limits<size_t>::max() ? 0.5f : m_VerticesStoreRef.getValue(nbrCellPtr->vertex_index * 3 + 2);
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);
            numNeighbors++;
          }
        }
      }

      // Add a fraction of the averaged vertex position to the current position
      if(numNeighbors > 0)
      {
        // Constrain vertex location to a max distance from the original voxel
        const float min = 0.5f - relaxAttrs.max_dist_from_cell_center;
        const float max = 0.5f + relaxAttrs.max_dist_from_cell_center;

        avgP[0] /= static_cast<float>(numNeighbors);
        avgP[1] /= static_cast<float>(numNeighbors);
        avgP[2] /= static_cast<float>(numNeighbors);
        const float alpha = relaxAttrs.relax_factor;

        float x = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3) + alpha * avgP[0];
        x = std::clamp(x, min, max);
        m_VerticesStoreRef.setValue(pCellPtr->vertex_index * 3, x);

        float y = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 1) + alpha * avgP[1];
        y = std::clamp(y, min, max);
        m_VerticesStoreRef.setValue(pCellPtr->vertex_index * 3 + 1, y);

        float z = (1.0f - alpha) * m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 2) + alpha * avgP[2];
        z = std::clamp(z, min, max);
        m_VerticesStoreRef.setValue(pCellPtr->vertex_index * 3 + 2, z);
      }
    }
  }
}

// Data for export
void MMCellMap::getArraySize(int arraySize[3]) const
{
  arraySize[0] = static_cast<int32_t>(m_ArraySize[0]);
  arraySize[1] = static_cast<int32_t>(m_ArraySize[1]);
  arraySize[2] = static_cast<int32_t>(m_ArraySize[2]);
}

void MMCellMap::getVoxelSize(float voxelSize[3]) const
{
  voxelSize[0] = m_VoxelSize[0];
  voxelSize[1] = m_VoxelSize[1];
  voxelSize[2] = m_VoxelSize[2];
}

size_t MMCellMap::numVertices() const
{
  return m_VertexArray.size();
}

size_t MMCellMap::numEdgeCrossings() const
{
  size_t cellMapIdx = 0;
  size_t numCrossings = 0;
  for(int k = 0; k < m_ArraySize[2]; k++)
  {
    for(int j = 0; j < m_ArraySize[1]; j++)
    {
      for(int i = 0; i < m_ArraySize[0]; i++)
      {
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::LeftBackEdge))
        {
          numCrossings++;
        }
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::LeftBottomEdge))
        {
          numCrossings++;
        }
        if(isEdgeCrossing(cellMapIdx, MMCellFlag::Edge::BackBottomEdge))
        {
          numCrossings++;
        }
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
  const int32_t* cellIndexPtr = vertex.cell_index;
  getVertexPosition(cellIndexPtr, position);
}

// The caller is responsible for bounds checking to allow for optimal performance.
void MMCellMap::getEdgeLabels(int cellIndex[3], const MMCellFlag::Edge edge, int32_t quadLabels[2], usize quadNxArrayIndices[2]) const
{
  std::array<int32_t, 3> firstCellLabels{};
  std::array<int32_t, 3> secondCellLabels{};

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
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2] + 1};
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
    secondCellLabels = {cellIndex[0] + 1, cellIndex[1], cellIndex[2] + 1};
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

  if(cellIndex[0] - 1 >= 0 && cellIndex[0] - 1 < m_ArraySize[0] - 1 && cellIndex[1] - 1 > 0 && cellIndex[1] - 1 < m_ArraySize[1] - 1 && cellIndex[2] - 1 > 0 && cellIndex[2] - 1 < m_ArraySize[2] - 1)
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
  Cell* pCellPtr = getCell(cellIndex);
  const int length = static_cast<int32_t>(m_ArraySize[0]);
  const int area = static_cast<int32_t>(m_ArraySize[0] * m_ArraySize[1]);
  quadVtxIndices[0] = pCellPtr->vertex_index;
  switch(edge)
  {
  case MMCellFlag::Edge::LeftBottomEdge:
    quadVtxIndices[1] = (pCellPtr - area)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - 1 - area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - 1)->vertex_index;
    break;
  case MMCellFlag::Edge::RightBottomEdge:
    quadVtxIndices[1] = (pCellPtr + 1)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + 1 - area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - area)->vertex_index;
    break;
  case MMCellFlag::Edge::BackBottomEdge:
    quadVtxIndices[1] = (pCellPtr - length)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - length - area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - area)->vertex_index;
    break;
  case MMCellFlag::Edge::FrontBottomEdge:
    quadVtxIndices[1] = (pCellPtr - area)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + length - area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + length)->vertex_index;
    break;
  case MMCellFlag::Edge::LeftTopEdge:
    quadVtxIndices[1] = (pCellPtr - 1)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - 1 + area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + area)->vertex_index;
    break;
  case MMCellFlag::Edge::RightTopEdge:
    quadVtxIndices[1] = (pCellPtr + area)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + 1 + area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + 1)->vertex_index;
    break;
  case MMCellFlag::Edge::BackTopEdge:
    quadVtxIndices[1] = (pCellPtr + area)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - length + area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - length)->vertex_index;
    break;
  case MMCellFlag::Edge::FrontTopEdge:
    quadVtxIndices[1] = (pCellPtr + length)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + length + area)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + area)->vertex_index;
    break;
  case MMCellFlag::Edge::LeftBackEdge:
    quadVtxIndices[1] = (pCellPtr - 1)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - 1 - length)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - length)->vertex_index;
    break;
  case MMCellFlag::Edge::RightBackEdge:
    quadVtxIndices[1] = (pCellPtr - length)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + 1 - length)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + 1)->vertex_index;
    break;
  case MMCellFlag::Edge::LeftFrontEdge:
    quadVtxIndices[1] = (pCellPtr + length)->vertex_index;
    quadVtxIndices[2] = (pCellPtr - 1 + length)->vertex_index;
    quadVtxIndices[3] = (pCellPtr - 1)->vertex_index;
    break;
  case MMCellFlag::Edge::RightFrontEdge:
    quadVtxIndices[1] = (pCellPtr + 1)->vertex_index;
    quadVtxIndices[2] = (pCellPtr + 1 + length)->vertex_index;
    quadVtxIndices[3] = (pCellPtr + length)->vertex_index;
    break;
  default:
    quadVtxIndices[1] = (pCellPtr)->vertex_index;
    quadVtxIndices[2] = (pCellPtr)->vertex_index;
    quadVtxIndices[3] = (pCellPtr)->vertex_index;
    break;
  }
}

// Access cell map. The caller is responsible for bounds checking.
MMCellMap::Cell* MMCellMap::getCell(int cellIndex[3]) const
{
  return (&(m_CellArray[cellArrayIndex(cellIndex)]));
}

MMCellMap::Cell* MMCellMap::getCell(int i, int j, int k) const
{
  return (&(m_CellArray[cellArrayIndex(i, j, k)]));
}

MMCellMap::Cell* MMCellMap::getCell(size_t cellMapIndex) const
{
  return (&(m_CellArray[cellMapIndex]));
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
  return (ii + m_ArraySize[0] * jj + m_ArraySize[0] * m_ArraySize[1] * kk);
}

void MMCellMap::getCornerLabels(const int cellIndex[3], int32_t labels[8]) const
{
  // The cell's 8 corners are the 8 voxels surrounding a grid corner, ordered
  // left-to-right, back-to-front, bottom-to-top. Must match the gather in setCellVertices().
  const int i = cellIndex[0];
  const int j = cellIndex[1];
  const int k = cellIndex[2];

  int32 corner[3] = {i, j, k};
  labels[0] = label(corner);

  corner[0] = i + 1;
  labels[1] = label(corner);

  corner[0] = i + 1;
  corner[1] = j + 1;
  labels[2] = label(corner);

  corner[0] = i;
  corner[1] = j + 1;
  labels[3] = label(corner);

  corner[0] = i;
  corner[1] = j;
  corner[2] = k + 1;
  labels[4] = label(corner);

  corner[0] = i + 1;
  corner[1] = j;
  corner[2] = k + 1;
  labels[5] = label(corner);

  corner[0] = i + 1;
  corner[1] = j + 1;
  corner[2] = k + 1;
  labels[6] = label(corner);

  corner[0] = i;
  corner[1] = j + 1;
  corner[2] = k + 1;
  labels[7] = label(corner);
}

int8_t MMCellMap::nodeType(const int cellIndex[3]) const
{
  int32_t labels[8];
  getCornerLabels(cellIndex, labels);

  // Distinct-label count with Padding counted as one member, exactly as QuickSurfaceMesh
  // counts -1 as a member of its owner list.
  int32_t distinct[8];
  int numDistinct = 0;
  bool hasPadding = false;
  for(int label : labels)
  {
    if(label == MMSurfaceNet::ReservedLabel::Padding)
    {
      hasPadding = true;
    }
    bool alreadySeen = false;
    for(int seenIdx = 0; seenIdx < numDistinct; seenIdx++)
    {
      if(distinct[seenIdx] == label)
      {
        alreadySeen = true;
        break;
      }
    }
    if(!alreadySeen)
    {
      distinct[numDistinct] = label;
      numDistinct++;
    }
  }

  int8_t result = static_cast<int8_t>(numDistinct > 4 ? 4 : numDistinct);
  if(hasPadding)
  {
    result = static_cast<int8_t>(result + 10);
  }
  return result;
}

bool MMCellMap::isEdgeCrossing(size_t cellMapIndex, MMCellFlag::Edge edge) const
{
  Cell* pCellPtr = getCell(cellMapIndex);
  return (pCellPtr->flag.isEdgeCrossing(edge));
}

MMCellFlag::VertexType MMCellMap::cellVertexType(size_t cellMapIndex) const
{
  Cell* pCellPtr = getCell(cellMapIndex);
  return (pCellPtr->flag.vertexType());
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
  const Vertex* pVertexPtr = &(m_VertexArray[vertexIndex]);
  cellIndex[0] = pVertexPtr->cell_index[0];
  cellIndex[1] = pVertexPtr->cell_index[1];
  cellIndex[2] = pVertexPtr->cell_index[2];
}

void MMCellMap::getVertexPosition(const int cellIndex[3], float position[3]) const
{
  const Cell* pCellPtr = getCell(cellArrayIndex(cellIndex));

  position[0] = m_VoxelSize[0] * (static_cast<float>(cellIndex[0]) + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3));
  position[1] = m_VoxelSize[1] * (static_cast<float>(cellIndex[1]) + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 1));
  position[2] = m_VoxelSize[2] * (static_cast<float>(cellIndex[2]) + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 2));
}

void MMCellMap::getVertexPosition(int i, int j, int k, float position[3]) const
{
  const Cell* pCellPtr = getCell(i, j, k);

  position[0] = m_VoxelSize[0] * (i + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3));
  position[1] = m_VoxelSize[1] * (j + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 1));
  position[2] = m_VoxelSize[2] * (k + m_VerticesStoreRef.getValue(pCellPtr->vertex_index * 3 + 2));
}

usize MMCellMap::vertexFaceNeighborVertexIndex(size_t vertexIndex, MMCellFlag::Face face) const
{
  const Vertex& vertex = m_VertexArray[vertexIndex];
  const int32_t* cellIndexPtr = vertex.cell_index;

  size_t cellMapIndex = cellArrayIndex(cellIndexPtr);
  switch(face)
  {
  case MMCellFlag::Face::LeftFace:
    return (cellMapIndex - 1);
  case MMCellFlag::Face::RightFace:
    return (cellMapIndex + 1);
  case MMCellFlag::Face::BackFace:
    return (cellMapIndex - m_ArraySize[0]);
  case MMCellFlag::Face::FrontFace:
    return (cellMapIndex + m_ArraySize[0]);
  case MMCellFlag::Face::BottomFace:
    return (cellMapIndex - m_ArraySize[0] * m_ArraySize[1]);
  case MMCellFlag::Face::TopFace:
    return (cellMapIndex + m_ArraySize[0] * m_ArraySize[1]);
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
  return (&(m_CellArray[cellArrayIndex(nbrCellIndex)]));
}

// MMCellMap.cpp
//
// MMCellMap implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <algorithm>
#include <cstdlib>
#include <exception>

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
: m_VerticesStoreRef(verticesStore)
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
    m_flagArray.resize(numCells, 0);
    m_vertexIndexArray.resize(numCells, std::numeric_limits<size_t>::max());
  } catch(std::bad_alloc&)
  {
    m_flagArray.clear();
    m_vertexIndexArray.clear();
    return;
  }
}

// ----------------------------------------------------------------------------
bool MMCellMap::valid() const
{
  return !m_flagArray.empty();
}

// ----------------------------------------------------------------------------
bool MMCellMap::init()
{
  // Vectors are already value-initialized in the constructor.
  // Set the cell vertices
  return setCellVertices();
}

// ----------------------------------------------------------------------------
bool MMCellMap::setCellVertices()
{
  // Get raw pointer to label data for optimized lookups
  const auto& labelStoreRef = m_NxLabelsPtr->getDataStoreRef();

  // Precompute strides for NX flat index calculation
  const size_t nxStrideY = m_NxDims[0];
  const size_t nxStrideZ = m_NxDims[0] * m_NxDims[1];

  // Precompute the 8 corner offsets relative to the NX flat index for cell (i,j,k)
  // Corner 0: (i-1, j-1, k-1) => offset 0 (base)
  // Corner 1: (i,   j-1, k-1) => offset +1
  // Corner 2: (i,   j,   k-1) => offset +1 + nxStrideY
  // Corner 3: (i-1, j,   k-1) => offset +nxStrideY
  // Corner 4: (i-1, j-1, k)   => offset +nxStrideZ
  // Corner 5: (i,   j-1, k)   => offset +1 + nxStrideZ
  // Corner 6: (i,   j,   k)   => offset +1 + nxStrideY + nxStrideZ
  // Corner 7: (i-1, j,   k)   => offset +nxStrideY + nxStrideZ
  const std::array<size_t, 8> cornerOffsets = {0, 1, 1 + nxStrideY, nxStrideY, nxStrideZ, 1 + nxStrideZ, 1 + nxStrideY + nxStrideZ, nxStrideY + nxStrideZ};

  // Set cell type and count cell vertices. There are no vertices in right, front,
  // top faces.
  size_t numVertices = 0;
  m_numEdgeCrossings = 0;
  const int iMax = static_cast<int>(m_arraySize[0]) - 1;
  const int jMax = static_cast<int>(m_arraySize[1]) - 1;
  const int kMax = static_cast<int>(m_arraySize[2]) - 1;

  for(int k = 0; k < kMax; k++)
  {
    for(int j = 0; j < jMax; j++)
    {
      for(int i = 0; i < iMax; i++)
      {
        size_t cellIdx = cellArrayIndex(i, j, k);
        int32_t cellLabels[8];

        // Check if all 8 corners are inside the NX volume (interior cell)
        // Interior cells have i in [1, iMax-1), j in [1, jMax-1), k in [1, kMax-1)
        if(i >= 1 && i < iMax - 1 && j >= 1 && j < jMax - 1 && k >= 1 && k < kMax - 1)
        {
          // All 8 corners are inside: use direct flat index arithmetic
          // NX index for corner 0 = (i-1) + (j-1)*nxStrideY + (k-1)*nxStrideZ
          size_t nxBase = static_cast<size_t>(i - 1) + static_cast<size_t>(j - 1) * nxStrideY + static_cast<size_t>(k - 1) * nxStrideZ;
          cellLabels[0] = labelStoreRef[nxBase + cornerOffsets[0]];
          cellLabels[1] = labelStoreRef[nxBase + cornerOffsets[1]];
          cellLabels[2] = labelStoreRef[nxBase + cornerOffsets[2]];
          cellLabels[3] = labelStoreRef[nxBase + cornerOffsets[3]];
          cellLabels[4] = labelStoreRef[nxBase + cornerOffsets[4]];
          cellLabels[5] = labelStoreRef[nxBase + cornerOffsets[5]];
          cellLabels[6] = labelStoreRef[nxBase + cornerOffsets[6]];
          cellLabels[7] = labelStoreRef[nxBase + cornerOffsets[7]];
        }
        else
        {
          // Boundary cell: use label() which handles padding
          int32 ci[3];
          ci[0] = i;
          ci[1] = j;
          ci[2] = k;
          cellLabels[0] = label(ci);

          ci[0] = i + 1;
          cellLabels[1] = label(ci);

          ci[0] = i + 1;
          ci[1] = j + 1;
          cellLabels[2] = label(ci);

          ci[0] = i;
          ci[1] = j + 1;
          cellLabels[3] = label(ci);

          ci[0] = i;
          ci[1] = j;
          ci[2] = k + 1;
          cellLabels[4] = label(ci);

          ci[0] = i + 1;
          ci[1] = j;
          ci[2] = k + 1;
          cellLabels[5] = label(ci);

          ci[0] = i + 1;
          ci[1] = j + 1;
          ci[2] = k + 1;
          cellLabels[6] = label(ci);

          ci[0] = i;
          ci[1] = j + 1;
          ci[2] = k + 1;
          cellLabels[7] = label(ci);
        }

        MMCellFlag::setFromLabels(m_flagArray[cellIdx], cellLabels);
        uint32_t flag = m_flagArray[cellIdx];
        if(MMCellFlag::vertexTypeFromFlag(flag) != MMCellFlag::VertexType::NoVertex)
        {
          numVertices++;
        }

        // Count edge crossings for the 3 edges we process per cell
        if(MMCellFlag::isEdgeCrossingFromFlag(flag, MMCellFlag::Edge::BackBottomEdge))
        {
          m_numEdgeCrossings++;
        }
        if(MMCellFlag::isEdgeCrossingFromFlag(flag, MMCellFlag::Edge::LeftBottomEdge))
        {
          m_numEdgeCrossings++;
        }
        if(MMCellFlag::isEdgeCrossingFromFlag(flag, MMCellFlag::Edge::LeftBackEdge))
        {
          m_numEdgeCrossings++;
        }
      }
    }
  }

  // Create cell vertices. There are no vertices in right, front, top faces.
  try
  {
    m_VerticesStoreRef.resizeTuples(ShapeType{numVertices});
    m_VertexArray.resize(numVertices);
  } catch(std::bad_alloc&)
  {
    m_flagArray.clear();
    m_vertexIndexArray.clear();
    return false;
  }
  int idxVtx = 0;
  for(int k = 0; k < kMax; k++)
  {
    for(int j = 0; j < jMax; j++)
    {
      for(int i = 0; i < iMax; i++)
      {
        size_t cellIdx = cellArrayIndex(i, j, k);
        if(MMCellFlag::vertexTypeFromFlag(m_flagArray[cellIdx]) != MMCellFlag::VertexType::NoVertex)
        {
          m_vertexIndexArray[cellIdx] = idxVtx;
          m_VertexArray[idxVtx].cellFlatIndex = cellIdx;

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

  int32_t lbl = 0x8BABABAB;
  if(cellIndex[0] - 1 >= 0 && cellIndex[0] - 1 < m_arraySize[0] - 1 && cellIndex[1] - 1 >= 0 && cellIndex[1] - 1 < m_arraySize[1] - 1 && cellIndex[2] - 1 >= 0 && cellIndex[2] - 1 < m_arraySize[2] - 1)
  {
    size_t nxArrayIdx = ToNxFlatIndex(cellIndex, m_NxDims.data());
    lbl = m_NxLabelsPtr->at(nxArrayIdx);
  }
  return lbl;
}

// Relax vertex positions using relaxation attributes or reset to cell centers
void MMCellMap::relax(const MMSurfaceNet::RelaxAttrs& relaxAttrs) const
{
  const size_t numVerts = m_VertexArray.size();
  if(numVerts == 0)
  {
    return;
  }

  // Step 6: Copy positions into a local buffer for better cache locality
  const size_t numFloats = numVerts * 3;
  std::vector<float> localPos(numFloats);
  for(size_t idx = 0; idx < numFloats; idx++)
  {
    localPos[idx] = m_VerticesStoreRef.getValue(idx);
  }

  for(int iter = 0; iter < relaxAttrs.numRelaxIterations; iter++)
  {
    for(size_t idxVtx = 0; idxVtx < numVerts; idxVtx++)
    {
      int cellIdx[3];
      getVertexCellIndex(idxVtx, cellIdx);
      size_t cellFlatIdx = cellArrayIndex(cellIdx);
      uint32_t flag = m_flagArray[cellFlatIdx];

      int numNeighbors = 0;
      float avgP[3] = {0.0f, 0.0f, 0.0f};
      MMCellFlag::VertexType vType = MMCellFlag::vertexTypeFromFlag(flag);

      if(vType == MMCellFlag::VertexType::SurfaceVertex)
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(MMCellFlag::faceCrossingTypeFromFlag(flag, face) != MMCellFlag::FaceCrossingType::NoFaceCrossing)
          {
            int nbrIdx[3];
            size_t nbrFlatIdx = getFaceNeighborAndIndex(cellIdx, face, nbrIdx);
            size_t nbrVertIdx = m_vertexIndexArray[nbrFlatIdx];

            float avgpX = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3];
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3 + 1];
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3 + 2];
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);

            numNeighbors++;
          }
        }
      }
      else
      {
        for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
        {
          if(MMCellFlag::faceCrossingTypeFromFlag(flag, face) == MMCellFlag::FaceCrossingType::JunctionFaceCrossing)
          {
            int nbrIdx[3];
            size_t nbrFlatIdx = getFaceNeighborAndIndex(cellIdx, face, nbrIdx);
            size_t nbrVertIdx = m_vertexIndexArray[nbrFlatIdx];

            float avgpX = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3];
            avgP[0] += avgpX + static_cast<float>(nbrIdx[0] - cellIdx[0]);

            float avgpY = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3 + 1];
            avgP[1] += avgpY + static_cast<float>(nbrIdx[1] - cellIdx[1]);

            float avgpZ = nbrVertIdx == std::numeric_limits<size_t>::max() ? 0.5f : localPos[nbrVertIdx * 3 + 2];
            avgP[2] += avgpZ + static_cast<float>(nbrIdx[2] - cellIdx[2]);
            numNeighbors++;
          }
        }
      }

      // Add a fraction of the averaged vertex position to the current position
      if(numNeighbors > 0)
      {
        size_t vtxIdx = m_vertexIndexArray[cellFlatIdx];
        // Constrain vertex location to a max distance from the original voxel
        const float min = 0.5f - relaxAttrs.maxDistFromCellCenter;
        const float max = 0.5f + relaxAttrs.maxDistFromCellCenter;

        avgP[0] /= static_cast<float>(numNeighbors);
        avgP[1] /= static_cast<float>(numNeighbors);
        avgP[2] /= static_cast<float>(numNeighbors);
        const float alpha = relaxAttrs.relaxFactor;

        float x = (1.0f - alpha) * localPos[vtxIdx * 3] + alpha * avgP[0];
        x = std::clamp(x, min, max);
        localPos[vtxIdx * 3] = x;

        float y = (1.0f - alpha) * localPos[vtxIdx * 3 + 1] + alpha * avgP[1];
        y = std::clamp(y, min, max);
        localPos[vtxIdx * 3 + 1] = y;

        float z = (1.0f - alpha) * localPos[vtxIdx * 3 + 2] + alpha * avgP[2];
        z = std::clamp(z, min, max);
        localPos[vtxIdx * 3 + 2] = z;
      }
    }
  }

  // Copy back to the DataStore
  for(size_t idx = 0; idx < numFloats; idx++)
  {
    m_VerticesStoreRef.setValue(idx, localPos[idx]);
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
  return m_numEdgeCrossings;
}

MMCellFlag::VertexType MMCellMap::vertexType(size_t vertexIndex) const
{
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  return cellVertexType(cellArrayIndex(cellIndex));
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
  int cellIndex[3];
  getVertexCellIndex(vertexIndex, cellIndex);
  getVertexPosition(cellIndex, position);
}

// SoA accessors
uint32_t MMCellMap::cellFlag(size_t cellArrayIdx) const
{
  return m_flagArray[cellArrayIdx];
}

size_t MMCellMap::cellVertexIndex(size_t cellArrayIdx) const
{
  return m_vertexIndexArray[cellArrayIdx];
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
void MMCellMap::getEdgeQuadVtxIndices(int cellIndex[3], MMCellFlag::Edge edge, size_t quadVtxIndices[4]) const
{
  size_t baseCellIdx = cellArrayIndex(cellIndex);
  const size_t length = m_arraySize[0];
  const size_t area = m_arraySize[0] * m_arraySize[1];

  quadVtxIndices[0] = m_vertexIndexArray[baseCellIdx];
  switch(edge)
  {
  case MMCellFlag::Edge::LeftBottomEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - area];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - 1 - area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - 1];
    break;
  case MMCellFlag::Edge::RightBottomEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + 1];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + 1 - area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - area];
    break;
  case MMCellFlag::Edge::BackBottomEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - length];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - length - area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - area];
    break;
  case MMCellFlag::Edge::FrontBottomEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - area];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + length - area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + length];
    break;
  case MMCellFlag::Edge::LeftTopEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - 1];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - 1 + area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + area];
    break;
  case MMCellFlag::Edge::RightTopEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + area];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + 1 + area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + 1];
    break;
  case MMCellFlag::Edge::BackTopEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + area];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - length + area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - length];
    break;
  case MMCellFlag::Edge::FrontTopEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + length];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + length + area];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + area];
    break;
  case MMCellFlag::Edge::LeftBackEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - 1];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - 1 - length];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - length];
    break;
  case MMCellFlag::Edge::RightBackEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx - length];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + 1 - length];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + 1];
    break;
  case MMCellFlag::Edge::LeftFrontEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + length];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx - 1 + length];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx - 1];
    break;
  case MMCellFlag::Edge::RightFrontEdge:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx + 1];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx + 1 + length];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx + length];
    break;
  default:
    quadVtxIndices[1] = m_vertexIndexArray[baseCellIdx];
    quadVtxIndices[2] = m_vertexIndexArray[baseCellIdx];
    quadVtxIndices[3] = m_vertexIndexArray[baseCellIdx];
    break;
  }
}

size_t MMCellMap::cellArrayIndex(const int cellIndex[3]) const
{
  return cellArrayIndex(cellIndex[0], cellIndex[1], cellIndex[2]);
}

size_t MMCellMap::cellArrayIndex(int i, int j, int k) const
{
  const auto ii = static_cast<size_t>(i);
  const auto jj = static_cast<size_t>(j);
  const auto kk = static_cast<size_t>(k);
  return (ii + m_arraySize[0] * jj + m_arraySize[0] * m_arraySize[1] * kk);
}

bool MMCellMap::isEdgeCrossing(size_t cellArrayIdx, MMCellFlag::Edge edge) const
{
  return MMCellFlag::isEdgeCrossingFromFlag(m_flagArray[cellArrayIdx], edge);
}

MMCellFlag::VertexType MMCellMap::cellVertexType(size_t cellArrayIdx) const
{
  return MMCellFlag::vertexTypeFromFlag(m_flagArray[cellArrayIdx]);
}

// Recover (i,j,k) from a flat cell index
void MMCellMap::cellIndexFromFlat(size_t flatIdx, int& i, int& j, int& k) const
{
  const size_t sliceSize = m_arraySize[0] * m_arraySize[1];
  k = static_cast<int>(flatIdx / sliceSize);
  size_t remainder = flatIdx % sliceSize;
  j = static_cast<int>(remainder / m_arraySize[0]);
  i = static_cast<int>(remainder % m_arraySize[0]);
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
  cellIndexFromFlat(m_VertexArray[vertexIndex].cellFlatIndex, cellIndex[0], cellIndex[1], cellIndex[2]);
}

void MMCellMap::getVertexPosition(const int cellIndex[3], float position[3]) const
{
  size_t cellIdx = cellArrayIndex(cellIndex);
  size_t vtxIdx = m_vertexIndexArray[cellIdx];

  position[0] = m_voxelSize[0] * (static_cast<float>(cellIndex[0]) + m_VerticesStoreRef.getValue(vtxIdx * 3));
  position[1] = m_voxelSize[1] * (static_cast<float>(cellIndex[1]) + m_VerticesStoreRef.getValue(vtxIdx * 3 + 1));
  position[2] = m_voxelSize[2] * (static_cast<float>(cellIndex[2]) + m_VerticesStoreRef.getValue(vtxIdx * 3 + 2));
}

// Access cell neighbors
size_t MMCellMap::getFaceNeighborAndIndex(const int cellIndex[3], MMCellFlag::Face face, int nbrCellIndex[3]) const
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
  return cellArrayIndex(nbrCellIndex);
}

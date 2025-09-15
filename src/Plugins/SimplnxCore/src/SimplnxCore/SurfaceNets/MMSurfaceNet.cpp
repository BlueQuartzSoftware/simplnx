// MMSurfaceNet.cpp
//
// Implementation of the MMSurfaceNet Application Programming Interface (API)
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include "MMCellMap.h"
// #include "MMGeometryGL.h"
// #include "MMGeometryOBJ.h"
#include "MMSurfaceNet.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <string>

#include <time.h>

MMSurfaceNet::MMSurfaceNet(TriangleGeom& triangleGeometry, Int32Array& labels, int arraySize[3], float voxelSize[3])
{
  m_cellMap = std::make_shared<MMCellMap>(triangleGeometry, arraySize, voxelSize);
  if(!m_cellMap->valid())
  {
    return;
  }
  m_cellMap->init(labels);
}

MMSurfaceNet::~MMSurfaceNet() = default;

// Surface smoothing (relaxation)
void MMSurfaceNet::relax(const RelaxAttrs relaxAttrs) const
{
  if(!m_cellMap)
    return;
  m_cellMap->relax(relaxAttrs);
}
void MMSurfaceNet::reset() const
{
  if(!m_cellMap)
    return;
  m_cellMap->reset();
}

#if 0
std::vector<int> MMSurfaceNet::labels()
{
  std::vector<int> labels;
  if(m_cellMap != nullptr)
  {
    // Find the unique material labels
    std::set<int> labelSet;
    for(int idxVtx = 0; idxVtx < m_cellMap->numVertices(); idxVtx++)
    {
      int vertexIndices[4];
      int32_t quadLabels[2];

      // Back-bottom edge
      if(m_cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices, quadLabels) == true)
      {
        labelSet.insert((int)quadLabels[0]);
        labelSet.insert((int)quadLabels[1]);
      }

      // Left-bottom edge
      if(m_cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices, quadLabels) == true)
      {
        labelSet.insert(quadLabels[0]);
        labelSet.insert(quadLabels[1]);
      }

      // Left-back edge
      if(m_cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices, quadLabels) == true)
      {
        labelSet.insert(quadLabels[0]);
        labelSet.insert(quadLabels[1]);
      }
    }
    // Removed the reserved padding index
    labelSet.erase(ReservedLabel::Padding);
    labels.assign(labelSet.begin(), labelSet.end());
  }

  return labels;
}
#endif

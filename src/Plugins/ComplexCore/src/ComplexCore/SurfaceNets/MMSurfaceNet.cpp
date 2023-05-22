// MMSurfaceNet.cpp
//
// Implementation of the MMSurfaceNet Application Programming Interface (API)
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <algorithm>
#include <cstdlib>
#include <set>
#include <string>
#include <time.h>

#include "MMCellMap.h"
#include "MMGeometryGL.h"
#include "MMGeometryOBJ.h"
#include "MMSurfaceNet.h"

MMSurfaceNet::MMSurfaceNet(int32_t* labels, int arraySize[3], float voxelSize[3])
: m_cellMap(nullptr)
{
  if(m_cellMap != NULL)
    delete m_cellMap;
  m_cellMap = new MMCellMap(labels, arraySize, voxelSize);
}
MMSurfaceNet::~MMSurfaceNet()
{
  // Delete cellMap if it exists
  if(m_cellMap)
    delete m_cellMap;
}

// Surface smoothing (relaxation)
void MMSurfaceNet::relax(const RelaxAttrs relaxAttrs)
{
  if(!m_cellMap)
    return;
  m_cellMap->relax(relaxAttrs);
}
void MMSurfaceNet::reset()
{
  if(!m_cellMap)
    return;
  m_cellMap->reset();
}

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

// MMGeometryGL.cpp
//
// MMGeometryGL implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <algorithm>

#include "MMCellFlag.h"
#include "MMCellMap.h"
#include "MMGeometryGL.h"
#include "MMSurfaceNet.h"

#include <cmath>

MMGeometryGL::MMGeometryGL(MMSurfaceNet* surfaceNet)
: m_origin{0, 0, 0}
, m_size{0, 0, 0}
, m_numVertices(0)
, m_numIndices(0)
, m_vertices(nullptr)
, m_indices(nullptr)
{
  if(surfaceNet == nullptr)
    return;
  MMCellMap* cellMap = surfaceNet->m_cellMap;
  if(!cellMap)
    return;

  int arraySize[3];
  float voxelSize[3];
  cellMap->getArraySize(arraySize);
  cellMap->getVoxelSize(voxelSize);
  for(int i = 0; i < 3; i++)
  {
    m_origin[i] = 0.0;
    m_size[i] = arraySize[i] * voxelSize[i];
  }

  // Allocate memory
  try
  {
    int numQuads = cellMap->numEdgeCrossings();
    int numVertsPerQuad = 4;
    int numFloatsPerVertex = sizeof(GLVertex) / sizeof(float);
    long int numFloats = numQuads * numVertsPerQuad * numFloatsPerVertex;
    m_vertices = new float[numFloats];
    int numIndicesPerQuad = 6;
    long int numIndices = numQuads * numIndicesPerQuad;
    m_indices = new unsigned int[numIndices];
  } catch(std::bad_alloc& ba)
  {
    if(m_vertices)
      delete[] m_vertices;
    if(m_indices)
      delete[] m_indices;
    m_vertices = nullptr;
    m_indices = nullptr;
    return;
  }

  // Make a mapping from each label to a texture coordinate for GL rendering
  std::vector<int> labels = surfaceNet->labels();
  for(int i = 0; i < labels.size(); i++)
  {
    m_labelToTexCoord.insert(std::make_pair(labels[i], float(i)));
  }

  // Construct geometry
  m_numVertices = 0;
  m_numIndices = 0;
  float* pVertices = m_vertices;
  unsigned int* pIndices = m_indices;
  for(int idxVtx = 0; idxVtx < cellMap->numVertices(); idxVtx++)
  {
    float vertexPositions[12];
    unsigned short labelsTmp[2];

    // Back-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexPositions, labelsTmp) == true)
    {
      MMGeometryGL::makeGLQuad(vertexPositions, labelsTmp, pVertices, pIndices, m_numVertices);
      pVertices += 4 * 8;
      pIndices += 6;
      m_numVertices += 4;
      m_numIndices += 6;
    }

    // Left-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexPositions, labelsTmp) == true)
    {
      MMGeometryGL::makeGLQuad(vertexPositions, labelsTmp, pVertices, pIndices, m_numVertices);
      pVertices += 4 * 8;
      pIndices += 6;
      m_numVertices += 4;
      m_numIndices += 6;
    }

    // Left-back edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexPositions, labelsTmp) == true)
    {
      MMGeometryGL::makeGLQuad(vertexPositions, labelsTmp, pVertices, pIndices, m_numVertices);
      pVertices += 4 * 8;
      pIndices += 6;
      m_numVertices += 4;
      m_numIndices += 6;
    }
  }
}

MMGeometryGL::~MMGeometryGL()
{
  delete[] m_vertices;
  delete[] m_indices;
}

void MMGeometryGL::origin(float origin[3])
{
  origin[0] = m_origin[0];
  origin[1] = m_origin[1];
  origin[2] = m_origin[2];
}
void MMGeometryGL::maxSize(float size[3])
{
  size[0] = m_size[0];
  size[1] = m_size[1];
  size[2] = m_size[2];
}

void MMGeometryGL::makeGLQuad(float* positions, unsigned short tissueLabels[2], float* quadVerts, unsigned int* quadIndices, int idxOffset)
{
  float norm[3];
  computeQuadNormal(positions, norm);
  float* pos = positions;
  float* pVert = quadVerts;
  for(int i = 0; i < 4; i++)
  {
    *pVert++ = *pos++;
    *pVert++ = *pos++;
    *pVert++ = *pos++;
    *pVert++ = norm[0];
    *pVert++ = norm[1];
    *pVert++ = norm[2];
    *pVert++ = m_labelToTexCoord[tissueLabels[0]]; // float(tissueLabels[0]);
    *pVert++ = m_labelToTexCoord[tissueLabels[1]]; // float(tissueLabels[1]);
  }
  quadIndices[0] = idxOffset + 0;
  quadIndices[1] = idxOffset + 1;
  quadIndices[2] = idxOffset + 2;
  quadIndices[3] = idxOffset + 0;
  quadIndices[4] = idxOffset + 2;
  quadIndices[5] = idxOffset + 3;
}

void MMGeometryGL::computeQuadNormal(float* positions, float* normal)
{
  float* p0 = positions;
  float* p1 = positions + 3;
  float* p2 = positions + 6;
  float* p3 = positions + 9;
  float v1[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
  float v2[3] = {p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2]};
  float crossProduct[3] = {v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]};
  float len = std::sqrt(crossProduct[0] * crossProduct[0] + crossProduct[1] * crossProduct[1] + crossProduct[2] * crossProduct[2]);
  if(len > 0.000001)
  {
    normal[0] = crossProduct[0] / len;
    normal[1] = crossProduct[1] / len;
    normal[2] = crossProduct[2] / len;
  }
  else
  {
    normal[0] = 0.0f;
    normal[1] = 0.0f;
    normal[2] = 0.0f;
  }
}

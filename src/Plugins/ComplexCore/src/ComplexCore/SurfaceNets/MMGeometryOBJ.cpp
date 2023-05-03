// MMGeometryOBJ.cpp
//
// MMGeometryOBJ implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include <array>
#include <cmath>
#include <map>
#include <vector>

#include "MMCellMap.h"
#include "MMGeometryOBJ.h"

//
// Private data class for storing quad data during OBJ data generation
//
class MMQuad
{
public:
  MMQuad()
  : m_vertexIndices{-1, -1, -1, -1}
  , m_labels{0, 0}
  {
  }
  MMQuad(int vi[4], int32_t labels[2])
  : m_vertexIndices{vi[0], vi[1], vi[2], vi[3]}
  , m_labels{labels[0], labels[1]}
  {
  }

  void getVertexIndices(int vertexIndices[4])
  {
    for(int i = 0; i < 4; i++)
      vertexIndices[i] = m_vertexIndices[i];
  }
  void getLabels(unsigned short labels[2])
  {
    for(int i = 0; i < 2; i++)
      labels[i] = m_labels[i];
  }
  void setVertexIndices(int vertexIndices[4])
  {
    for(int i = 0; i < 4; i++)
      m_vertexIndices[i] = vertexIndices[i];
  }
  void setLabels(unsigned short labels[2])
  {
    for(int i = 0; i < 2; i++)
      m_labels[i] = labels[i];
  }

private:
  int m_vertexIndices[4];
  int32_t m_labels[2];
};

//
// MMGeometryOBJ implementation
//
MMGeometryOBJ::MMGeometryOBJ(MMSurfaceNet* surfaceNet)
: m_surfaceNet(surfaceNet)
{
  if(m_surfaceNet == nullptr)
    return;
  MMCellMap* cellMap = surfaceNet->m_cellMap;
  if(cellMap == nullptr)
    return;

  // Create temporary storage for cell quads which are constructed around edges
  // crossed by the surface. Handle 3 edges per cell. The other 9 cell edges will
  // be handled when neighboring cells that share edges with this cell are visited.
  for(int idxVtx = 0; idxVtx < cellMap->numVertices(); idxVtx++)
  {
    int vertexIndices[4];
    int32_t quadLabels[2];

    // Back-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices, quadLabels) == true)
    {
      MMQuad quad(vertexIndices, quadLabels);
      m_quads.push_back(quad);
    }

    // Left-bottom edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices, quadLabels) == true)
    {
      MMQuad quad(vertexIndices, quadLabels);
      m_quads.push_back(quad);
    }

    // Left-back edge
    if(cellMap->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices, quadLabels) == true)
    {
      MMQuad quad(vertexIndices, quadLabels);
      m_quads.push_back(quad);
    }
  }
}
MMGeometryOBJ::~MMGeometryOBJ()
{
}

std::vector<int> MMGeometryOBJ::labels()
{
  return m_surfaceNet->labels();
}
MMGeometryOBJ::OBJData MMGeometryOBJ::objData(int label)
{
  OBJData output;

  // Initialize a dictionary of vertex data for quads that touch this material
  std::map<int, vtxData> vtxDataMap; // key: vertexIndex, value: vtxData for this vertex
  for(std::vector<MMQuad>::iterator itQuad = m_quads.begin(); itQuad != m_quads.end(); itQuad++)
  {
    unsigned short quadLabels[2];
    itQuad->getLabels(quadLabels);
    if(label == quadLabels[0] || label == quadLabels[1])
    {
      int quadVtxIndices[4];
      itQuad->getVertexIndices(quadVtxIndices);
      for(int i = 0; i < 4; i++)
      {
        vtxData vDataInit = {-1, 0, 0, 0};
        vtxDataMap.insert(std::make_pair(quadVtxIndices[i], vDataInit));
      }
    }
  }

  // Update the dictionary of unique vertices with OBJ vertex index and position. Store
  // vertex position in the output. OBJ vertex indexing starts at 1 (OBJ convention)
  // rather than 0 (C++ convention)
  int vID = 1;
  MMCellMap* cellMap = m_surfaceNet->m_cellMap;
  for(std::map<int, vtxData>::iterator itVtxData = vtxDataMap.begin(); itVtxData != vtxDataMap.end(); itVtxData++)
  {
    float position[3];
    int cellIdx = itVtxData->first;
    cellMap->getVertexPosition(cellIdx, position);
    itVtxData->second = {vID++, position[0], position[1], position[2]};
    std::array<float, 3> p({position[0], position[1], position[2]});
    output.vertexPositions.push_back(p);
  }

  // Get face vertex indices (two triangles per quad) and store them in the output.
  for(std::vector<MMQuad>::iterator itQuad = m_quads.begin(); itQuad != m_quads.end(); itQuad++)
  {
    int quadVtxIndices[4];
    unsigned short quadLabels[2];
    itQuad->getLabels(quadLabels);
    itQuad->getVertexIndices(quadVtxIndices);
    if(label == quadLabels[0] || label == quadLabels[1])
    {
      vtxData vData[4];
      for(int i = 0; i < 4; i++)
      {
        vData[i] = vtxDataMap[quadVtxIndices[i]];
      }
      bool isQuadFrontFacing = (label == quadLabels[0]) ? true : false;
      int triangleVtxIDs[6];
      MMGeometryOBJ::getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      std::array<int, 3> t1({triangleVtxIDs[0], triangleVtxIDs[1], triangleVtxIDs[2]});
      std::array<int, 3> t2({triangleVtxIDs[3], triangleVtxIDs[4], triangleVtxIDs[5]});
      output.triangles.push_back(t1);
      output.triangles.push_back(t2);
    }
  }

  return (output);
}

void crossProduct(float v0[3], float v1[3], float result[3])
{
  // Cross product of vectors v0 and v1
  result[0] = v0[1] * v1[2] - v0[2] * v1[1];
  result[1] = v0[2] * v1[0] - v0[0] * v1[2];
  result[2] = v0[0] * v1[1] - v0[1] * v1[0];
}
float triangleArea(float p0[3], float p1[3], float p2[3])
{
  // Area of triangle with vertex positions p0, p1, p2
  float v01[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
  float v02[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
  float cp[3];
  crossProduct(v01, v02, cp);
  float magCP = std::sqrt(cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2]);
  return 0.5 * magCP;
}

void MMGeometryOBJ::getQuadTriangleIDs(vtxData vData[4], bool isQuadFrontFacing, int triangleVtxIDs[6])
{
  // Order quad vertices so quad is front facing
  if(!isQuadFrontFacing)
  {
    vtxData temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Order quad vertices so that the two generated triangles have the minimal area. This
  // reduces self intersections in the surface.
  float thisArea = triangleArea(vData[0].position, vData[1].position, vData[2].position) + triangleArea(vData[0].position, vData[2].position, vData[3].position);
  float alternateArea = triangleArea(vData[1].position, vData[2].position, vData[3].position) + triangleArea(vData[1].position, vData[3].position, vData[0].position);
  if(alternateArea < thisArea)
  {
    vtxData temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Generate vertex ids to triangulate the quad
  triangleVtxIDs[0] = vData[0].vID;
  triangleVtxIDs[1] = vData[1].vID;
  triangleVtxIDs[2] = vData[2].vID;
  triangleVtxIDs[3] = vData[0].vID;
  triangleVtxIDs[4] = vData[2].vID;
  triangleVtxIDs[5] = vData[3].vID;
}

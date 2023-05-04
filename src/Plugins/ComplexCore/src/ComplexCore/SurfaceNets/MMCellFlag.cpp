// MMCellFlag.cpp
//
// MMCellFlag implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include "MMCellFlag.h"
#include <type_traits>

MMCellFlag::MMCellFlag()
: m_bitFlag(0)
{
}
MMCellFlag::~MMCellFlag()
{
}

void MMCellFlag::set(unsigned short cellLabels[8])
{
  // By default the cell has no vertex and no face or edge crossings
  m_bitFlag = 0;

  // Find edge crossings
  int numEdgeCrossings = 0;
  if(cellLabels[0] != cellLabels[3])
  {
    m_bitFlag |= m_leftBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[1] != cellLabels[2])
  {
    m_bitFlag |= m_rightBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[0] != cellLabels[1])
  {
    m_bitFlag |= m_backBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[2] != cellLabels[3])
  {
    m_bitFlag |= m_frontBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[4] != cellLabels[7])
  {
    m_bitFlag |= m_leftTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[5] != cellLabels[6])
  {
    m_bitFlag |= m_rightTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[4] != cellLabels[5])
  {
    m_bitFlag |= m_backTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[6] != cellLabels[7])
  {
    m_bitFlag |= m_frontTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[0] != cellLabels[4])
  {
    m_bitFlag |= m_leftBackEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[1] != cellLabels[5])
  {
    m_bitFlag |= m_rightBackEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[3] != cellLabels[7])
  {
    m_bitFlag |= m_leftFrontEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[2] != cellLabels[6])
  {
    m_bitFlag |= m_rightFrontEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(numEdgeCrossings == 0)
    return;

  // Find face crossings
  unsigned int faceTypeBits;
  faceTypeBits = faceCrossingTypeAsBits(cellLabels[0], cellLabels[3], cellLabels[7], cellLabels[4]);
  m_bitFlag |= (faceTypeBits << LeftFaceShift);
  faceTypeBits = faceCrossingTypeAsBits(cellLabels[1], cellLabels[2], cellLabels[6], cellLabels[5]);
  m_bitFlag |= (faceTypeBits << RightFaceShift);

  faceTypeBits = faceCrossingTypeAsBits(cellLabels[0], cellLabels[1], cellLabels[5], cellLabels[4]);
  m_bitFlag |= (faceTypeBits << BackFaceShift);
  faceTypeBits = faceCrossingTypeAsBits(cellLabels[3], cellLabels[2], cellLabels[6], cellLabels[7]);
  m_bitFlag |= (faceTypeBits << FrontFaceShift);

  faceTypeBits = faceCrossingTypeAsBits(cellLabels[0], cellLabels[1], cellLabels[2], cellLabels[3]);
  m_bitFlag |= (faceTypeBits << BottomFaceShift);
  faceTypeBits = faceCrossingTypeAsBits(cellLabels[4], cellLabels[5], cellLabels[6], cellLabels[7]);
  m_bitFlag |= (faceTypeBits << TopFaceShift);

  // Determine vertex type
  int numFaceCrossings = 0;
  m_numJunctions = 0;
  for(Face face = Face::LeftFace; face <= Face::TopFace; ++face)
  {
    if(faceCrossingType(face) != FaceCrossingType::NoFaceCrossing)
    {
      numFaceCrossings++;
      if(faceCrossingType(face) == FaceCrossingType::JunctionFaceCrossing)
      {
        m_numJunctions++;
      }
    }
  }
  if(numFaceCrossings != 0)
  {
    unsigned int vertexTypeBits = 0;
    if(m_numJunctions < 1)
    {
      vertexTypeBits = (unsigned int)VertexType::SurfaceVertex;
    }
    else if(m_numJunctions <= 2)
    {
      vertexTypeBits = (unsigned int)VertexType::EdgeVertex;
    }
    else
    {
      vertexTypeBits = (unsigned int)VertexType::CornerVertex;
    }
    m_bitFlag |= (vertexTypeBits << VertexTypeShift);
  }
}

unsigned char MMCellFlag::numJunctions() const
{
  return m_numJunctions;
}

MMCellFlag::VertexType MMCellFlag::vertexType()
{
  unsigned int vertexTypeBits = (m_bitFlag & m_vertexTypeBits) >> VertexTypeShift;
  switch(vertexTypeBits)
  {
  case 0:
    return (VertexType::NoVertex);
  case 1:
    return (VertexType::SurfaceVertex);
  case 2:
    return (VertexType::EdgeVertex);
  case 3:
    return (VertexType::CornerVertex);
  default:
    return (VertexType::NoVertex);
  }
}
MMCellFlag::FaceCrossingType MMCellFlag::faceCrossingType(Face face)
{
  unsigned int faceTypeBits = 0;
  switch(face)
  {
  case Face::LeftFace:
    faceTypeBits = (m_bitFlag & m_leftFaceCrossingBits) >> LeftFaceShift;
    break;
  case Face::RightFace:
    faceTypeBits = (m_bitFlag & m_rightFaceCrossingBits) >> RightFaceShift;
    break;
  case Face::BackFace:
    faceTypeBits = (m_bitFlag & m_backFaceCrossingBits) >> BackFaceShift;
    break;
  case Face::FrontFace:
    faceTypeBits = (m_bitFlag & m_frontFaceCrossingBits) >> FrontFaceShift;
    break;
  case Face::BottomFace:
    faceTypeBits = (m_bitFlag & m_bottomFaceCrossingBits) >> BottomFaceShift;
    break;
  case Face::TopFace:
    faceTypeBits = (m_bitFlag & m_topFaceCrossingBits) >> TopFaceShift;
    break;
  default:
    faceTypeBits = 0;
  }

  switch(faceTypeBits)
  {
  case 0:
    return (FaceCrossingType::NoFaceCrossing);
  case 1:
    return (FaceCrossingType::SurfaceFaceCrossing);
  case 2:
    return (FaceCrossingType::JunctionFaceCrossing);
  default:
    return (FaceCrossingType::NoFaceCrossing);
  }
}
bool MMCellFlag::isEdgeCrossing(Edge edge)
{
  switch(edge)
  {
  case Edge::LeftBottomEdge: {
    if((m_bitFlag & m_leftBottomEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::RightBottomEdge: {
    if((m_bitFlag & m_rightBottomEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::BackBottomEdge: {
    if((m_bitFlag & m_backBottomEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::FrontBottomEdge: {
    if((m_bitFlag & m_frontBottomEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::LeftTopEdge: {
    if((m_bitFlag & m_leftTopEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::RightTopEdge: {
    if((m_bitFlag & m_rightTopEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::BackTopEdge: {
    if((m_bitFlag & m_backTopEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::FrontTopEdge: {
    if((m_bitFlag & m_frontTopEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::LeftBackEdge: {
    if((m_bitFlag & m_leftBackEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::RightBackEdge: {
    if((m_bitFlag & m_rightBackEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::LeftFrontEdge: {
    if((m_bitFlag & m_leftFrontEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  case Edge::RightFrontEdge: {
    if((m_bitFlag & m_rightFrontEdgeCrossingBit) > 0)
      return true;
    return false;
  }
  default: {
    return (false);
  }
  }
}

unsigned int MMCellFlag::faceCrossingTypeAsBits(unsigned short c0, unsigned short c1, unsigned short c2, unsigned short c3)
{
  int numUniqueTypes = 0;
  unsigned short uniqueTypes[4];
  uniqueTypes[numUniqueTypes++] = c0;
  if(c1 != uniqueTypes[0])
    uniqueTypes[numUniqueTypes++] = c1;
  int idx = 0;
  while(idx < numUniqueTypes && c2 != uniqueTypes[idx])
    idx++;
  if(idx == numUniqueTypes)
    uniqueTypes[numUniqueTypes++] = c2;
  idx = 0;
  while(idx < numUniqueTypes && c3 != uniqueTypes[idx])
    idx++;
  if(idx == numUniqueTypes)
    uniqueTypes[numUniqueTypes++] = c3;
  FaceCrossingType crossingType = FaceCrossingType::NoFaceCrossing;
  switch(numUniqueTypes)
  {
  case 0:
  case 1:
    crossingType = FaceCrossingType::NoFaceCrossing;
    break;
  case 2: {
    if(c0 == c2 && c1 == c3)
      crossingType = FaceCrossingType::JunctionFaceCrossing;
    else
      crossingType = FaceCrossingType::SurfaceFaceCrossing;
    break;
  }
  case 3:
  case 4:
    crossingType = FaceCrossingType::JunctionFaceCrossing;
    break;
  default:
    crossingType = FaceCrossingType::NoFaceCrossing;
    break;
  }
  return (unsigned int)crossingType;
}

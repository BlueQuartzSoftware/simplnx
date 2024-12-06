// MMCellFlag.cpp
//
// MMCellFlag implementation
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#include "MMCellFlag.h"

#include <array>

namespace
{
// Bit shifts to locate various components of the cell flag
constexpr uint32_t k_VertexTypeShift = 0;
constexpr uint32_t k_LeftFaceShift = 2;
constexpr uint32_t k_RightFaceShift = 4;
constexpr uint32_t k_BackFaceShift = 6;
constexpr uint32_t k_FrontFaceShift = 8;
constexpr uint32_t k_BottomFaceShift = 10;
constexpr uint32_t k_TopFaceShift = 12;

// Flag bits associated with each component of the cell flag
constexpr uint32_t k_VertexTypeBits = (1 << k_VertexTypeShift) | (1 << (k_VertexTypeShift + 1));
constexpr uint32_t k_LeftFaceCrossingBits = (1 << k_LeftFaceShift) | (1 << (k_LeftFaceShift + 1));
constexpr uint32_t k_RightFaceCrossingBits = (1 << k_RightFaceShift) | (1 << (k_RightFaceShift + 1));
constexpr uint32_t k_BackFaceCrossingBits = (1 << k_BackFaceShift) | (1 << (k_BackFaceShift + 1));
constexpr uint32_t k_FrontFaceCrossingBits = (1 << k_FrontFaceShift) | (1 << (k_FrontFaceShift + 1));
constexpr uint32_t k_BottomFaceCrossingBits = (1 << k_BottomFaceShift) | (1 << (k_BottomFaceShift + 1));
constexpr uint32_t k_TopFaceCrossingBits = (1 << k_TopFaceShift) | (1 << (k_TopFaceShift + 1));
constexpr uint32_t k_LeftBottomEdgeCrossingBit = 1 << 14;
constexpr uint32_t k_RightBottomEdgeCrossingBit = 1 << 15;
constexpr uint32_t k_BackBottomEdgeCrossingBit = 1 << 16;
constexpr uint32_t k_FrontBottomEdgeCrossingBit = 1 << 17;
constexpr uint32_t k_LeftTopEdgeCrossingBit = 1 << 18;
constexpr uint32_t k_RightTopEdgeCrossingBit = 1 << 19;
constexpr uint32_t k_BackTopEdgeCrossingBit = 1 << 20;
constexpr uint32_t k_FrontTopEdgeCrossingBit = 1 << 21;
constexpr uint32_t k_LeftBackEdgeCrossingBit = 1 << 22;
constexpr uint32_t k_RightBackEdgeCrossingBit = 1 << 23;
constexpr uint32_t k_LeftFrontEdgeCrossingBit = 1 << 24;
constexpr uint32_t k_RightFrontEdgeCrossingBit = 1 << 25;

uint32_t FaceCrossingTypeAsBits(int32_t c0, int32_t c1, int32_t c2, int32_t c3)
{
  int32_t numUniqueTypes = 0;
  std::array<int32_t, 4> uniqueTypes;
  uniqueTypes[numUniqueTypes++] = c0;
  if(c1 != uniqueTypes[0])
  {
    uniqueTypes[numUniqueTypes++] = c1;
  }
  int32_t idx = 0;
  while(idx < numUniqueTypes && c2 != uniqueTypes[idx])
  {
    idx++;
  }
  if(idx == numUniqueTypes)
  {
    uniqueTypes[numUniqueTypes++] = c2;
  }
  idx = 0;
  while(idx < numUniqueTypes && c3 != uniqueTypes[idx])
  {
    idx++;
  }
  if(idx == numUniqueTypes)
  {
    uniqueTypes[numUniqueTypes++] = c3;
  }
  MMCellFlag::FaceCrossingType crossingType = MMCellFlag::FaceCrossingType::NoFaceCrossing;
  switch(numUniqueTypes)
  {
  case 0: {
    [[fallthrough]];
  }
  case 1: {
    crossingType = MMCellFlag::FaceCrossingType::NoFaceCrossing;
    break;
  }
  case 2: {
    if(c0 == c2 && c1 == c3)
    {
      crossingType = MMCellFlag::FaceCrossingType::JunctionFaceCrossing;
    }
    else
    {
      crossingType = MMCellFlag::FaceCrossingType::SurfaceFaceCrossing;
    }
    break;
  }
  case 3: {
    [[fallthrough]];
  }
  case 4: {
    crossingType = MMCellFlag::FaceCrossingType::JunctionFaceCrossing;
    break;
  }
  default: {
    crossingType = MMCellFlag::FaceCrossingType::NoFaceCrossing;
    break;
  }
  }
  return static_cast<uint32_t>(crossingType);
}
} // namespace

void MMCellFlag::set(const int32_t cellLabels[8])
{
  // By default the cell has no vertex and no face or edge crossings
  m_BitFlag = 0;

  // Find edge crossings
  int32_t numEdgeCrossings = 0;
  if(cellLabels[0] != cellLabels[3])
  {
    m_BitFlag |= k_LeftBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[1] != cellLabels[2])
  {
    m_BitFlag |= k_RightBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[0] != cellLabels[1])
  {
    m_BitFlag |= k_BackBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[2] != cellLabels[3])
  {
    m_BitFlag |= k_FrontBottomEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[4] != cellLabels[7])
  {
    m_BitFlag |= k_LeftTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[5] != cellLabels[6])
  {
    m_BitFlag |= k_RightTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[4] != cellLabels[5])
  {
    m_BitFlag |= k_BackTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[6] != cellLabels[7])
  {
    m_BitFlag |= k_FrontTopEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[0] != cellLabels[4])
  {
    m_BitFlag |= k_LeftBackEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[1] != cellLabels[5])
  {
    m_BitFlag |= k_RightBackEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[3] != cellLabels[7])
  {
    m_BitFlag |= k_LeftFrontEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(cellLabels[2] != cellLabels[6])
  {
    m_BitFlag |= k_RightFrontEdgeCrossingBit;
    numEdgeCrossings++;
  }
  if(numEdgeCrossings == 0)
  {
    return;
  }

  // Find face crossings
  uint32_t faceTypeBits = FaceCrossingTypeAsBits(cellLabels[0], cellLabels[3], cellLabels[7], cellLabels[4]);
  m_BitFlag |= (faceTypeBits << k_LeftFaceShift);
  faceTypeBits = FaceCrossingTypeAsBits(cellLabels[1], cellLabels[2], cellLabels[6], cellLabels[5]);
  m_BitFlag |= (faceTypeBits << k_RightFaceShift);

  faceTypeBits = FaceCrossingTypeAsBits(cellLabels[0], cellLabels[1], cellLabels[5], cellLabels[4]);
  m_BitFlag |= (faceTypeBits << k_BackFaceShift);
  faceTypeBits = FaceCrossingTypeAsBits(cellLabels[3], cellLabels[2], cellLabels[6], cellLabels[7]);
  m_BitFlag |= (faceTypeBits << k_FrontFaceShift);

  faceTypeBits = FaceCrossingTypeAsBits(cellLabels[0], cellLabels[1], cellLabels[2], cellLabels[3]);
  m_BitFlag |= (faceTypeBits << k_BottomFaceShift);
  faceTypeBits = FaceCrossingTypeAsBits(cellLabels[4], cellLabels[5], cellLabels[6], cellLabels[7]);
  m_BitFlag |= (faceTypeBits << k_TopFaceShift);

  // Determine vertex type
  int32_t numFaceCrossings = 0;
  uint32_t numJunctions = 0;
  for(Face face = Face::LeftFace; face <= Face::TopFace; ++face)
  {
    if(faceCrossingType(face) != FaceCrossingType::NoFaceCrossing)
    {
      numFaceCrossings++;
      if(faceCrossingType(face) == FaceCrossingType::JunctionFaceCrossing)
      {
        numJunctions++;
      }
    }
  }
  if(numFaceCrossings != 0)
  {
    uint32_t vertexTypeBits = 0;
    if(numJunctions < 1)
    {
      vertexTypeBits = static_cast<uint32_t>(VertexType::SurfaceVertex);
    }
    else if(numJunctions <= 2)
    {
      vertexTypeBits = static_cast<uint32_t>(VertexType::EdgeVertex);
    }
    else
    {
      vertexTypeBits = static_cast<uint32_t>(VertexType::CornerVertex);
    }
    m_BitFlag |= (vertexTypeBits << k_VertexTypeShift);
  }

  m_BitFlag |= numJunctions << k_NumJunctionsBitShift;
}

MMCellFlag::VertexType MMCellFlag::vertexType() const
{
  uint32_t vertexTypeBits = (m_BitFlag & k_VertexTypeBits) >> k_VertexTypeShift;
  switch(vertexTypeBits)
  {
  case 0: {
    return VertexType::NoVertex;
  }
  case 1: {
    return VertexType::SurfaceVertex;
  }
  case 2: {
    return VertexType::EdgeVertex;
  }
  case 3: {
    return VertexType::CornerVertex;
  }
  default: {
    return VertexType::NoVertex;
  }
  }
}
MMCellFlag::FaceCrossingType MMCellFlag::faceCrossingType(Face face) const
{
  uint32_t faceTypeBits = 0;
  switch(face)
  {
  case Face::LeftFace: {
    faceTypeBits = (m_BitFlag & k_LeftFaceCrossingBits) >> k_LeftFaceShift;
    break;
  }
  case Face::RightFace: {
    faceTypeBits = (m_BitFlag & k_RightFaceCrossingBits) >> k_RightFaceShift;
    break;
  }
  case Face::BackFace: {
    faceTypeBits = (m_BitFlag & k_BackFaceCrossingBits) >> k_BackFaceShift;
    break;
  }
  case Face::FrontFace: {
    faceTypeBits = (m_BitFlag & k_FrontFaceCrossingBits) >> k_FrontFaceShift;
    break;
  }
  case Face::BottomFace: {
    faceTypeBits = (m_BitFlag & k_BottomFaceCrossingBits) >> k_BottomFaceShift;
    break;
  }
  case Face::TopFace: {
    faceTypeBits = (m_BitFlag & k_TopFaceCrossingBits) >> k_TopFaceShift;
    break;
  }
  default: {
    faceTypeBits = 0;
    break;
  }
  }

  switch(faceTypeBits)
  {
  case 0: {
    return FaceCrossingType::NoFaceCrossing;
  }
  case 1: {
    return FaceCrossingType::SurfaceFaceCrossing;
  }
  case 2: {
    return FaceCrossingType::JunctionFaceCrossing;
  }
  default: {
    return FaceCrossingType::NoFaceCrossing;
  }
  }
}
bool MMCellFlag::isEdgeCrossing(Edge edge) const
{
  switch(edge)
  {
  case Edge::LeftBottomEdge: {
    return (m_BitFlag & k_LeftBottomEdgeCrossingBit) > 0;
  }
  case Edge::RightBottomEdge: {
    return (m_BitFlag & k_RightBottomEdgeCrossingBit) > 0;
  }
  case Edge::BackBottomEdge: {
    return (m_BitFlag & k_BackBottomEdgeCrossingBit) > 0;
  }
  case Edge::FrontBottomEdge: {
    return (m_BitFlag & k_FrontBottomEdgeCrossingBit) > 0;
  }
  case Edge::LeftTopEdge: {
    return (m_BitFlag & k_LeftTopEdgeCrossingBit) > 0;
  }
  case Edge::RightTopEdge: {
    return (m_BitFlag & k_RightTopEdgeCrossingBit) > 0;
  }
  case Edge::BackTopEdge: {
    return (m_BitFlag & k_BackTopEdgeCrossingBit) > 0;
  }
  case Edge::FrontTopEdge: {
    return (m_BitFlag & k_FrontTopEdgeCrossingBit) > 0;
  }
  case Edge::LeftBackEdge: {
    return (m_BitFlag & k_LeftBackEdgeCrossingBit) > 0;
  }
  case Edge::RightBackEdge: {
    return (m_BitFlag & k_RightBackEdgeCrossingBit) > 0;
  }
  case Edge::LeftFrontEdge: {
    return (m_BitFlag & k_LeftFrontEdgeCrossingBit) > 0;
  }
  case Edge::RightFrontEdge: {
    return (m_BitFlag & k_RightFrontEdgeCrossingBit) > 0;
  }
  default: {
    return false;
  }
  }
}

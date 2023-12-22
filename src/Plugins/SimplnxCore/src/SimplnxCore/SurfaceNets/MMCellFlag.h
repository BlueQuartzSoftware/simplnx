// MMCellFlag.h
//
// Interface for MMCellFlags, which are used for encoding local data for each cell
// and quick access to this data
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#ifndef MM_CELL_FLAG_H
#define MM_CELL_FLAG_H

#include <cstdint>

class MMCellFlag
{
public:
  MMCellFlag();
  virtual ~MMCellFlag();

  void operator=(const MMCellFlag& t)
  {
    m_bitFlag = t.m_bitFlag;
  }

  unsigned int getBitFlag() const
  {
    return m_bitFlag;
  }

  enum class VertexType
  {
    NoVertex,
    SurfaceVertex,
    EdgeVertex,
    CornerVertex
  };
  enum class FaceCrossingType
  {
    NoFaceCrossing,
    SurfaceFaceCrossing,
    JunctionFaceCrossing
  };
  enum class Face
  {
    LeftFace,
    RightFace,
    BackFace,
    FrontFace,
    BottomFace,
    TopFace
  };
  enum class Edge
  {
    LeftBottomEdge,
    RightBottomEdge,
    BackBottomEdge,
    FrontBottomEdge,
    LeftTopEdge,
    RightTopEdge,
    BackTopEdge,
    FrontTopEdge,
    LeftBackEdge,
    RightBackEdge,
    LeftFrontEdge,
    RightFrontEdge
  };

  // Set components of the cell flag. The components are determined from the
  // cell labels of its 8 vertices, which are listed left-to-right, back-to-front,
  // bottom-to-top (i.e., left-back-bottom vertex first and right-front-top vertex
  // last). Clearing the cell flag encodes types NoVertex, NoFaceCrossing, and no
  // edge or face crossings.
  void set(int32_t cellLabels[8]);

  void clear()
  {
    m_bitFlag = 0;
  }

  // Get components of the cell flag
  VertexType vertexType();
  FaceCrossingType faceCrossingType(Face face);
  bool isEdgeCrossing(Edge edge);
  unsigned char numJunctions() const;

private:
  // Bit shifts to locate various components of the cell flag
  enum BitShift
  {
    VertexTypeShift = 0,
    LeftFaceShift = 2,
    RightFaceShift = 4,
    BackFaceShift = 6,
    FrontFaceShift = 8,
    BottomFaceShift = 10,
    TopFaceShift = 12,
  };

  // Flag bits associated with each component of the cell flag
  const unsigned int m_vertexTypeBits = (1 << VertexTypeShift) | (1 << (VertexTypeShift + 1));
  const unsigned int m_leftFaceCrossingBits = (1 << LeftFaceShift) | (1 << (LeftFaceShift + 1));
  const unsigned int m_rightFaceCrossingBits = (1 << RightFaceShift) | (1 << (RightFaceShift + 1));
  const unsigned int m_backFaceCrossingBits = (1 << BackFaceShift) | (1 << (BackFaceShift + 1));
  const unsigned int m_frontFaceCrossingBits = (1 << FrontFaceShift) | (1 << (FrontFaceShift + 1));
  const unsigned int m_bottomFaceCrossingBits = (1 << BottomFaceShift) | (1 << (BottomFaceShift + 1));
  const unsigned int m_topFaceCrossingBits = (1 << TopFaceShift) | (1 << (TopFaceShift + 1));
  const unsigned int m_leftBottomEdgeCrossingBit = 1 << 14;
  const unsigned int m_rightBottomEdgeCrossingBit = 1 << 15;
  const unsigned int m_backBottomEdgeCrossingBit = 1 << 16;
  const unsigned int m_frontBottomEdgeCrossingBit = 1 << 17;
  const unsigned int m_leftTopEdgeCrossingBit = 1 << 18;
  const unsigned int m_rightTopEdgeCrossingBit = 1 << 19;
  const unsigned int m_backTopEdgeCrossingBit = 1 << 20;
  const unsigned int m_frontTopEdgeCrossingBit = 1 << 21;
  const unsigned int m_leftBackEdgeCrossingBit = 1 << 22;
  const unsigned int m_rightBackEdgeCrossingBit = 1 << 23;
  const unsigned int m_leftFrontEdgeCrossingBit = 1 << 24;
  const unsigned int m_rightFrontEdgeCrossingBit = 1 << 25;

  // The bitflag
  unsigned int m_bitFlag;
  unsigned char m_numJunctions = 0;

  // Determine face crossing type from the face's vertex labels
  unsigned int faceCrossingTypeAsBits(int32_t c0, int32_t c1, int32_t c2, int32_t c3);
};

// For iterating over cell faces
inline MMCellFlag::Face& operator++(MMCellFlag::Face& f)
{
  f = MMCellFlag::Face((unsigned int)(f) + 1);
  return f;
}
inline MMCellFlag::Face operator++(MMCellFlag::Face& f, int)
{
  f = MMCellFlag::Face((unsigned int)(f) + 1);
  return f;
}

#endif

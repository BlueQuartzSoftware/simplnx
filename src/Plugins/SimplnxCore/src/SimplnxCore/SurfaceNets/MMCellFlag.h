// MMCellFlag.h
//
// Interface for MMCellFlags, which are used for encoding local data for each cell
// and quick access to this data
//
// Sarah Frisken, Brigham and Women's Hospital, Boston MA USA

#pragma once

#include <cstdint>

class MMCellFlag
{
public:
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
  void set(const int32_t cellLabels[8]);

  uint32_t getBitFlag() const
  {
    return m_BitFlag;
  }

  void clear()
  {
    m_BitFlag = 0;
  }

  // Get components of the cell flag
  VertexType vertexType() const;
  FaceCrossingType faceCrossingType(Face face) const;
  bool isEdgeCrossing(Edge edge) const;
  uint8_t numJunctions() const
  {
    return m_BitFlag >> k_NumJunctionsBitShift;
  }

private:
  static inline constexpr uint32_t k_NumJunctionsBitShift = 29;

  // The bitflag
  // The last 3 bits of the bitflag are the number of junctions
  // numJunctions can at most be 6
  uint32_t m_BitFlag = 0;
};

// For iterating over cell faces
inline MMCellFlag::Face& operator++(MMCellFlag::Face& f)
{
  f = static_cast<MMCellFlag::Face>(static_cast<uint32_t>(f) + 1);
  return f;
}
inline MMCellFlag::Face& operator++(MMCellFlag::Face& f, int)
{
  f = static_cast<MMCellFlag::Face>(static_cast<uint32_t>(f) + 1);
  return f;
}

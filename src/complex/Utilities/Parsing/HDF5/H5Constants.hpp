#pragma once

#include "complex/Common/StringLiteral.hpp"

namespace complex
{
namespace H5Constants
{
// Grid Montage
inline constexpr StringLiteral k_RowCountTag = "Row Count";
inline constexpr StringLiteral k_ColCountTag = "Column Count";
inline constexpr StringLiteral k_DepthCountTag = "Depth Count";

// INodeGeometry0D

// INodeGeometry1D
inline constexpr StringLiteral k_FaceListTag = "Face List ID";
inline constexpr StringLiteral k_UnsharedEdgeListTag2 = "Unshared Edge List ID";

// INodeGeometry2D

// INodeGeometry3D

// Image Geometry
inline constexpr StringLiteral k_H5_DIMENSIONS = "_DIMENSIONS";
inline constexpr StringLiteral k_H5_ORIGIN = "_ORIGIN";
inline constexpr StringLiteral k_H5_SPACING = "_SPACING";
inline constexpr StringLiteral k_VoxelSizesTag = "Voxel Sizes ID";

// Rectilinear Geometry
inline constexpr StringLiteral k_XBoundsTag = "X Bounds ID";
inline constexpr StringLiteral k_YBoundsTag = "Y Bounds ID";
inline constexpr StringLiteral k_ZBoundsTag = "Z Bounds ID";
// inline constexpr StringLiteral k_VoxelSizesTag = "Voxel Sizes ID";
inline constexpr StringLiteral k_DimensionsTag = "Dimensions";

// Vertex Geometry
inline constexpr StringLiteral k_VertexListTag = "Vertex List ID";
inline constexpr StringLiteral k_VertexSizesTag = "Vertex Sizes ID";

// Edge Geometry
inline constexpr StringLiteral k_EdgeListTag = "Edge List ID";
inline constexpr StringLiteral k_EdgesContainingVertTag = "Edges Containing Vertex ID";
inline constexpr StringLiteral k_EdgeNeighborsTag = "Edge Neighbors ID";
inline constexpr StringLiteral k_EdgeCentroidTag = "Edge Centroids ID";
inline constexpr StringLiteral k_EdgeSizesTag = "Edge Sizes ID";
inline constexpr StringLiteral k_UnsharedEdgeListTag = "Unshared Edge List ID";

// Triangle Geometry
inline constexpr StringLiteral k_TriangleListTag = "Triangle List ID";
inline constexpr StringLiteral k_TrianglesContainingVertTag = "Triangles Containing Vertex ID";
inline constexpr StringLiteral k_TriangleNeighborsTag = "Triangle Neighbors ID";
inline constexpr StringLiteral k_TriangleCentroidsTag = "Triangle Centroids ID";
inline constexpr StringLiteral k_TriangleSizesTag = "Triangle Sizes ID";

// Quad Geometry
inline constexpr StringLiteral k_QuadListTag = "Quad List ID";
inline constexpr StringLiteral k_QuadsContainingVertTag = "Quads Containing Vertex ID";
inline constexpr StringLiteral k_QuadNeighborsTag = "Quad Neighbors ID";
inline constexpr StringLiteral k_QuadCentroidsTag = "Quad Centroids ID";
inline constexpr StringLiteral k_QuadSizesTag = "Quad Sizes ID";

// Hexahedral Geometry
inline constexpr StringLiteral k_UnsharedFaceeListTag = "Unshared Face List ID";
inline constexpr StringLiteral k_FaceeListTag = "Face List ID";

inline constexpr StringLiteral k_HexListTag = "Hexahedral List ID";
inline constexpr StringLiteral k_HexasContainingVertTag = "Hexahedrals Containing Vertex ID";
inline constexpr StringLiteral k_HexNeighborsTag = "Hexahedral Neighbors ID";
inline constexpr StringLiteral k_HexCentroidsTag = "Hexahedral Centroids ID";
inline constexpr StringLiteral k_HexSizesTag = "Hexahedral Sizes ID";

// Tetrahedral Geometry
inline constexpr StringLiteral k_TriListTag = "Triangle List ID";
inline constexpr StringLiteral k_UnsharedTriListTag = "Unshared Triangle List ID";
inline constexpr StringLiteral k_TetListTag = "Tetrahedral List ID";
inline constexpr StringLiteral k_TetsContainingVertTag = "Tetrahedrals Containing Vertex ID";
inline constexpr StringLiteral k_TetNeighborsTag = "Tetrahedral Neighbors ID";
inline constexpr StringLiteral k_TetCentroidsTag = "Tetrahedral Centroids ID";
inline constexpr StringLiteral k_TetSizesTag = "Tetrahedral Sizes ID";
} // namespace H5Constants
} // namespace complex

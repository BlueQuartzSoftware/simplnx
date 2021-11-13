#pragma once

#include "complex/Common/StringLiteral.hpp"

namespace complex
{
namespace H5Constants
{

// Grid Montage
static inline constexpr StringLiteral k_RowCountTag = "Row Count";
static inline constexpr StringLiteral k_ColCountTag = "Column Count";
static inline constexpr StringLiteral k_DepthCountTag = "Depth Count";

// Image Geometry
static inline constexpr StringLiteral k_H5_DIMENSIONS = "_DIMENSIONS";
static inline constexpr StringLiteral k_H5_ORIGIN = "_ORIGIN";
static inline constexpr StringLiteral k_H5_SPACING = "_SPACING";
static inline constexpr StringLiteral k_VoxelSizesTag = "Voxel Sizes ID";

// Rectilinear Geometry
static inline constexpr StringLiteral k_XBoundsTag = "X Bounds ID";
static inline constexpr StringLiteral k_YBoundsTag = "Y Bounds ID";
static inline constexpr StringLiteral k_ZBoundsTag = "Z Bounds ID";
// static inline constexpr StringLiteral k_VoxelSizesTag = "Voxel Sizes ID";
static inline constexpr StringLiteral k_DimensionsTag = "Dimensions";

// Vertex Geometry
static inline constexpr StringLiteral k_VertexListTag = "Vertex List ID";
static inline constexpr StringLiteral k_VertexSizesTag = "Vertex Sizes ID";

// Edge Geometry
static inline constexpr StringLiteral k_EdgeListTag = "Edge List ID";
static inline constexpr StringLiteral k_EdgesContainingVertTag = "Edges Containing Vertex ID";
static inline constexpr StringLiteral k_EdgeNeighborsTag = "Edge Neighbors ID";
static inline constexpr StringLiteral k_EdgeCentroidTag = "Edge Centroids ID";
static inline constexpr StringLiteral k_EdgeSizesTag = "Edge Sizes ID";
static inline constexpr StringLiteral k_UnsharedEdgeListTag = "Unshared Edge List ID";

// Triangle Geometry
static inline constexpr StringLiteral k_TriangleListTag = "Triangle List ID";
static inline constexpr StringLiteral k_TrianglesContainingVertTag = "Triangles Containing Vertex ID";
static inline constexpr StringLiteral k_TriangleNeighborsTag = "Triangle Neighbors ID";
static inline constexpr StringLiteral k_TriangleCentroidsTag = "Triangle Centroids ID";
static inline constexpr StringLiteral k_TriangleSizesTag = "Triangle Sizes ID";

// Quad Geometry
static inline constexpr StringLiteral k_QuadListTag = "Quad List ID";
static inline constexpr StringLiteral k_QuadsContainingVertTag = "Quads Containing Vertex ID";
static inline constexpr StringLiteral k_QuadNeighborsTag = "Quad Neighbors ID";
static inline constexpr StringLiteral k_QuadCentroidsTag = "Quad Centroids ID";
static inline constexpr StringLiteral k_QuadSizesTag = "Quad Sizes ID";

// Hexahedral Geometry
static inline constexpr StringLiteral k_UnsharedFaceeListTag = "Unshared Face List ID";
static inline constexpr StringLiteral k_FaceeListTag = "Face List ID";

static inline constexpr StringLiteral k_HexListTag = "Hexahedral List ID";
static inline constexpr StringLiteral k_HexasContainingVertTag = "Hexahedrals Containing Vertex ID";
static inline constexpr StringLiteral k_HexNeighborsTag = "Hexahedral Neighbors ID";
static inline constexpr StringLiteral k_HexCentroidsTag = "Hexahedral Centroids ID";
static inline constexpr StringLiteral k_HexSizesTag = "Hexahedral Sizes ID";

// Tetrahedral Geometry
static inline constexpr StringLiteral k_TriListTag = "Triangle List ID";
static inline constexpr StringLiteral k_UnsharedTriListTag = "Unshared Triangle List ID";
static inline constexpr StringLiteral k_TetListTag = "Tetrahedral List ID";
static inline constexpr StringLiteral k_TetsContainingVertTag = "Tetrahedrals Containing Vertex ID";
static inline constexpr StringLiteral k_TetNeighborsTag = "Tetrahedral Neighbors ID";
static inline constexpr StringLiteral k_TetCentroidsTag = "Tetrahedral Centroids ID";
static inline constexpr StringLiteral k_TetSizesTag = "Tetrahedral Sizes ID";

} // namespace H5Constants
} // namespace complex

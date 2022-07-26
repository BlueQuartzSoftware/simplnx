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

// IGeometry
inline constexpr StringLiteral k_ElementSizesTag = "Element Sizes ID";

// INodeGeometry0D
inline constexpr StringLiteral k_VertexListTag = "Vertex List ID";

// INodeGeometry1D
inline constexpr StringLiteral k_EdgeListTag = "Edge List ID";
inline constexpr StringLiteral k_ElementContainingVertTag = "Element Containing Vertex ID";
inline constexpr StringLiteral k_ElementNeighborsTag = "Element Neighbors ID";
inline constexpr StringLiteral k_ElementCentroidTag = "Element Centroids ID";

// INodeGeometry2D
inline constexpr StringLiteral k_FaceListTag = "Face List ID";
inline constexpr StringLiteral k_UnsharedEdgeListTag = "Unshared Edge List ID";

// INodeGeometry3D
inline constexpr StringLiteral k_PolyhedraListTag = "Polyhedra List ID";
inline constexpr StringLiteral k_UnsharedFaceListTag = "Unshared Face List ID";

// IGridGeometry
inline constexpr StringLiteral k_CellDataTag = "Cell Data ID";

// Image Geometry
inline constexpr StringLiteral k_H5_DIMENSIONS = "_DIMENSIONS";
inline constexpr StringLiteral k_H5_ORIGIN = "_ORIGIN";
inline constexpr StringLiteral k_H5_SPACING = "_SPACING";
inline constexpr StringLiteral k_VoxelSizesTag = "Voxel Sizes ID";

// Rectilinear Geometry
inline constexpr StringLiteral k_XBoundsTag = "X Bounds ID";
inline constexpr StringLiteral k_YBoundsTag = "Y Bounds ID";
inline constexpr StringLiteral k_ZBoundsTag = "Z Bounds ID";
inline constexpr StringLiteral k_DimensionsTag = "Dimensions";
} // namespace H5Constants
} // namespace complex

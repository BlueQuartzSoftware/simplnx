#pragma once

#include "simplnx/Common/StringLiteral.hpp"

namespace nx::core::io_constants
{
// DataArray
inline constexpr StringLiteralType k_TupleShapeTag = "TupleDimensions";
inline constexpr StringLiteralType k_ComponentShapeTag = "ComponentDimensions";

// AttributeMatrix
inline constexpr StringLiteralType k_TupleDims = "TupleDims";

// Grid Montage
inline constexpr StringLiteralType k_RowCountTag = "Row Count";
inline constexpr StringLiteralType k_ColCountTag = "Column Count";
inline constexpr StringLiteralType k_DepthCountTag = "Depth Count";

// IGeometry
inline constexpr StringLiteralType k_ElementSizesTag = "Element Sizes ID";
inline constexpr StringLiteralType k_H5Units = "Units";

// INodeGeometry0D
inline constexpr StringLiteralType k_VertexListTag = "Vertex List ID";
inline constexpr StringLiteralType k_VertexDataTag = "Vertex Data ID";

// INodeGeometry1D
inline constexpr StringLiteralType k_EdgeListTag = "Edge List ID";
inline constexpr StringLiteralType k_EdgeDataTag = "Edge Data ID";
inline constexpr StringLiteralType k_ElementContainingVertTag = "Element Containing Vertex ID";
inline constexpr StringLiteralType k_ElementNeighborsTag = "Element Neighbors ID";
inline constexpr StringLiteralType k_ElementCentroidTag = "Element Centroids ID";

// INodeGeometry2D
inline constexpr StringLiteralType k_FaceListTag = "Face List ID";
inline constexpr StringLiteralType k_FaceDataTag = "Face Data ID";
inline constexpr StringLiteralType k_UnsharedEdgeListTag = "Unshared Edge List ID";

// INodeGeometry3D
inline constexpr StringLiteralType k_PolyhedronListTag = "Polyhedron List ID";
inline constexpr StringLiteralType k_PolyhedronDataTag = "Polyhedron Data ID";
inline constexpr StringLiteralType k_UnsharedFaceListTag = "Unshared Face List ID";

// IGridGeometry
inline constexpr StringLiteralType k_CellDataTag = "Cell Data ID";

// Image Geometry
inline constexpr StringLiteralType k_H5Dimensions = "_DIMENSIONS";
inline constexpr StringLiteralType k_H5Origin = "_ORIGIN";
inline constexpr StringLiteralType k_H5Spacing = "_SPACING";
inline constexpr StringLiteralType k_VoxelSizesTag = "Voxel Sizes ID";

// Rectilinear Geometry
inline constexpr StringLiteralType k_XBoundsTag = "X Bounds ID";
inline constexpr StringLiteralType k_YBoundsTag = "Y Bounds ID";
inline constexpr StringLiteralType k_ZBoundsTag = "Z Bounds ID";
inline constexpr StringLiteralType k_DimensionsTag = "Dimensions";
} // namespace nx::core::io_constants

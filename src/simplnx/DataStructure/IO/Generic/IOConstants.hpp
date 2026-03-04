#pragma once

#include "simplnx/Common/StringLiteral.hpp"

namespace nx::core::IOConstants
{
// DataArray
inline constexpr StringLiteral k_TupleShapeTag = "TupleDimensions";
inline constexpr StringLiteral k_ComponentShapeTag = "ComponentDimensions";

// AttributeMatrix
inline constexpr StringLiteral k_TupleDims = "TupleDims";

// Grid Montage
inline constexpr StringLiteral k_RowCountTag = "Row Count";
inline constexpr StringLiteral k_ColCountTag = "Column Count";
inline constexpr StringLiteral k_DepthCountTag = "Depth Count";

// AbstractGeometry
inline constexpr StringLiteral k_ElementSizesTag = "Element Sizes ID";
inline constexpr StringLiteral k_H5_UNITS = "Units";

// AbstractNodeGeometry0D
inline constexpr StringLiteral k_VertexListTag = "Vertex List ID";
inline constexpr StringLiteral k_VertexDataTag = "Vertex Data ID";

// AbstractNodeGeometry1D
inline constexpr StringLiteral k_EdgeListTag = "Edge List ID";
inline constexpr StringLiteral k_EdgeDataTag = "Edge Data ID";
inline constexpr StringLiteral k_ElementContainingVertTag = "Element Containing Vertex ID";
inline constexpr StringLiteral k_ElementNeighborsTag = "Element Neighbors ID";
inline constexpr StringLiteral k_ElementCentroidTag = "Element Centroids ID";

// AbstractNodeGeometry2D
inline constexpr StringLiteral k_FaceListTag = "Face List ID";
inline constexpr StringLiteral k_FaceDataTag = "Face Data ID";
inline constexpr StringLiteral k_UnsharedEdgeListTag = "Unshared Edge List ID";

// AbstractNodeGeometry3D
inline constexpr StringLiteral k_PolyhedronListTag = "Polyhedron List ID";
inline constexpr StringLiteral k_PolyhedronDataTag = "Polyhedron Data ID";
inline constexpr StringLiteral k_UnsharedFaceListTag = "Unshared Face List ID";

// AbstractGridGeometry
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
} // namespace nx::core::IOConstants

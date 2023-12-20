#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/Montage/AbstractMontage.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

namespace nx::core
{
/**
 * @brief Returns a string representation of the passed in IGeometry::Type
 * @param dataType
 * @return
 */
inline constexpr StringLiteral GeometryTypeToString(IGeometry::Type geomType)
{
  switch(geomType)
  {
  case IGeometry::Type::Image: {
    return "Image";
  }
  case IGeometry::Type::RectGrid: {
    return "Rectilinear Grid";
  }
  case IGeometry::Type::Vertex: {
    return "Vertex";
  }
  case IGeometry::Type::Edge: {
    return "Edge";
  }
  case IGeometry::Type::Triangle: {
    return "Triangle";
  }
  case IGeometry::Type::Quad: {
    return "Quadrilateral";
  }
  case IGeometry::Type::Tetrahedral: {
    return "Tetrahedral";
  }
  case IGeometry::Type::Hexahedral: {
    return "Hexahedral";
  }
  default:
    throw std::runtime_error("nx::core::GeometryTypeToString: Unknown IGeometry::Type");
  }
}

/**
 * @brief Returns string representations for all DataTypes.
 * @return
 */
inline const std::vector<std::string>& GetAllGeometryTypesAsStrings()
{
  static const std::vector<std::string> geomTypes = {GeometryTypeToString(IGeometry::Type::Image),       GeometryTypeToString(IGeometry::Type::RectGrid),
                                                     GeometryTypeToString(IGeometry::Type::Vertex),      GeometryTypeToString(IGeometry::Type::Edge),
                                                     GeometryTypeToString(IGeometry::Type::Triangle),    GeometryTypeToString(IGeometry::Type::Quad),
                                                     GeometryTypeToString(IGeometry::Type::Tetrahedral), GeometryTypeToString(IGeometry::Type::Hexahedral)};
  return geomTypes;
}

/**
 * @brief Returns a IGeometry::Type for the passed in string representation
 * @param geomTypeString
 * @return
 */
inline constexpr IGeometry::Type StringToGeometryType(std::string_view geomTypeString)
{
  if(geomTypeString == GeometryTypeToString(IGeometry::Type::Image).view())
  {
    return IGeometry::Type::Image;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::RectGrid).view())
  {
    return IGeometry::Type::RectGrid;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Vertex).view())
  {
    return IGeometry::Type::Vertex;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Edge).view())
  {
    return IGeometry::Type::Edge;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Triangle).view())
  {
    return IGeometry::Type::Triangle;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Quad).view())
  {
    return IGeometry::Type::Quad;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Tetrahedral).view())
  {
    return IGeometry::Type::Tetrahedral;
  }
  else if(geomTypeString == GeometryTypeToString(IGeometry::Type::Hexahedral).view())
  {
    return IGeometry::Type::Hexahedral;
  }
  else
  {
    throw std::runtime_error("nx::core::StringToGeometryType: No known IGeometry::Type matches the given string value.");
  }
}

inline constexpr StringLiteral DataObjectTypeToString(DataObject::Type dataObjType)
{
  switch(dataObjType)
  {
  case nx::core::DataObject::Type::BaseGroup: {
    return nx::core::BaseGroup::k_TypeName;
  }
  case nx::core::DataObject::Type::DataGroup: {
    return nx::core::DataGroup::k_TypeName;
  }
  case nx::core::DataObject::Type::AttributeMatrix: {
    return nx::core::AttributeMatrix::k_TypeName;
  }
  case nx::core::DataObject::Type::IGeometry: {
    return nx::core::IGeometry::k_TypeName;
  }
  case nx::core::DataObject::Type::IGridGeometry: {
    return nx::core::IGridGeometry::k_TypeName;
  }
  case nx::core::DataObject::Type::INodeGeometry0D: {
    return nx::core::INodeGeometry0D::k_TypeName;
  }
  case nx::core::DataObject::Type::INodeGeometry1D: {
    return nx::core::INodeGeometry1D::k_TypeName;
  }
  case nx::core::DataObject::Type::INodeGeometry2D: {
    return nx::core::INodeGeometry2D::k_TypeName;
  }
  case nx::core::DataObject::Type::INodeGeometry3D: {
    return nx::core::INodeGeometry3D::k_TypeName;
  }
  case nx::core::DataObject::Type::ImageGeom: {
    return nx::core::ImageGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::RectGridGeom: {
    return nx::core::RectGridGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::VertexGeom: {
    return nx::core::VertexGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::EdgeGeom: {
    return nx::core::EdgeGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::TriangleGeom: {
    return nx::core::TriangleGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::QuadGeom: {
    return nx::core::QuadGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::TetrahedralGeom: {
    return nx::core::TetrahedralGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::HexahedralGeom: {
    return nx::core::HexahedralGeom::k_TypeName;
  }
  case nx::core::DataObject::Type::IDataArray: {
    return nx::core::IDataArray::k_TypeName;
  }
  case nx::core::DataObject::Type::DataArray: {
    return nx::core::DataArrayConstants::k_TypeName;
  }
  case nx::core::DataObject::Type::INeighborList: {
    return nx::core::INeighborList::k_TypeName;
  }
  case nx::core::DataObject::Type::NeighborList: {
    return nx::core::NeighborListConstants::k_TypeName;
  }
  case nx::core::DataObject::Type::ScalarData: {
    return nx::core::ScalarDataConstants::k_TypeName;
  }
  case nx::core::DataObject::Type::StringArray: {
    return nx::core::StringArray::k_TypeName;
  }
  case nx::core::DataObject::Type::DynamicListArray: {
    return nx::core::DynamicListArrayConstants::k_TypeName;
  }
  case nx::core::DataObject::Type::AbstractMontage: {
    return nx::core::AbstractMontage::k_TypeName;
  }
  case nx::core::DataObject::Type::GridMontage: {
    return nx::core::GridMontage::k_TypeName;
  }
  case nx::core::DataObject::Type::Any: {
    return {"Any"};
  }
  default: {
    throw std::runtime_error("nx::core::DataObjectTypeToString: Unknown DataObject::Type");
  }
  }
}

/**
 * @brief Converts IArray::ArrayType to DataObject::Type. ArrayType is a subset of DataObject::Type so this function cannot fail.
 * @param arrayType
 * @return
 */
inline constexpr DataObject::Type ConvertArrayTypeToDataObjectType(IArray::ArrayType arrayType)
{
  switch(arrayType)
  {
  case IArray::ArrayType::DataArray: {
    return DataObject::Type::DataArray;
  }
  case IArray::ArrayType::NeighborListArray: {
    return DataObject::Type::NeighborList;
  }
  case IArray::ArrayType::StringArray: {
    return DataObject::Type::StringArray;
  }
  case IArray::ArrayType::Any: {
    return DataObject::Type::Any;
  }
  default: {
    throw std::runtime_error("nx::core::ConvertArrayTypeToDataObjectType: Invalid ArrayType");
  }
  }
}

/**
 * @brief Converts DataObject::Type to IArray::ArrayType.
 * @param dataObjectType
 * @return
 */
inline constexpr std::optional<IArray::ArrayType> ConvertDataObjectTypeToArrayType(DataObject::Type dataObjectType) noexcept
{
  switch(dataObjectType)
  {
  case DataObject::Type::IDataArray:
  case DataObject::Type::DataArray: {
    return IArray::ArrayType::DataArray;
  }
  case DataObject::Type::INeighborList:
  case DataObject::Type::NeighborList: {
    return IArray::ArrayType::NeighborListArray;
  }
  case DataObject::Type::StringArray: {
    return IArray::ArrayType::StringArray;
  }
  case DataObject::Type::Any: {
    return IArray::ArrayType::Any;
  }
  default: {
    return {};
  }
  }
}
} // namespace nx::core

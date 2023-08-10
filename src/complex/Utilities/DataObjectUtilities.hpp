#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IArray.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/ScalarData.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/DataStructure/Montage/AbstractMontage.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

namespace complex
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
    throw std::runtime_error("complex::GeometryTypeToString: Unknown IGeometry::Type");
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
    throw std::runtime_error("complex::StringToGeometryType: No known IGeometry::Type matches the given string value.");
  }
}

inline constexpr StringLiteral DataObjectTypeToString(DataObject::Type dataObjType)
{
  switch(dataObjType)
  {
  case complex::DataObject::Type::BaseGroup: {
    return complex::BaseGroup::k_TypeName;
  }
  case complex::DataObject::Type::DataGroup: {
    return complex::DataGroup::k_TypeName;
  }
  case complex::DataObject::Type::AttributeMatrix: {
    return complex::AttributeMatrix::k_TypeName;
  }
  case complex::DataObject::Type::IGeometry: {
    return complex::IGeometry::k_TypeName;
  }
  case complex::DataObject::Type::IGridGeometry: {
    return complex::IGridGeometry::k_TypeName;
  }
  case complex::DataObject::Type::INodeGeometry0D: {
    return complex::INodeGeometry0D::k_TypeName;
  }
  case complex::DataObject::Type::INodeGeometry1D: {
    return complex::INodeGeometry1D::k_TypeName;
  }
  case complex::DataObject::Type::INodeGeometry2D: {
    return complex::INodeGeometry2D::k_TypeName;
  }
  case complex::DataObject::Type::INodeGeometry3D: {
    return complex::INodeGeometry3D::k_TypeName;
  }
  case complex::DataObject::Type::ImageGeom: {
    return complex::ImageGeom::k_TypeName;
  }
  case complex::DataObject::Type::RectGridGeom: {
    return complex::RectGridGeom::k_TypeName;
  }
  case complex::DataObject::Type::VertexGeom: {
    return complex::VertexGeom::k_TypeName;
  }
  case complex::DataObject::Type::EdgeGeom: {
    return complex::EdgeGeom::k_TypeName;
  }
  case complex::DataObject::Type::TriangleGeom: {
    return complex::TriangleGeom::k_TypeName;
  }
  case complex::DataObject::Type::QuadGeom: {
    return complex::QuadGeom::k_TypeName;
  }
  case complex::DataObject::Type::TetrahedralGeom: {
    return complex::TetrahedralGeom::k_TypeName;
  }
  case complex::DataObject::Type::HexahedralGeom: {
    return complex::HexahedralGeom::k_TypeName;
  }
  case complex::DataObject::Type::IDataArray: {
    return complex::IDataArray::k_TypeName;
  }
  case complex::DataObject::Type::DataArray: {
    return complex::DataArrayConstants::k_TypeName;
  }
  case complex::DataObject::Type::INeighborList: {
    return complex::INeighborList::k_TypeName;
  }
  case complex::DataObject::Type::NeighborList: {
    return complex::NeighborListConstants::k_TypeName;
  }
  case complex::DataObject::Type::ScalarData: {
    return complex::ScalarDataConstants::k_TypeName;
  }
  case complex::DataObject::Type::StringArray: {
    return complex::StringArray::k_TypeName;
  }
  case complex::DataObject::Type::DynamicListArray: {
    return complex::DynamicListArrayConstants::k_TypeName;
  }
  case complex::DataObject::Type::AbstractMontage: {
    return complex::AbstractMontage::k_TypeName;
  }
  case complex::DataObject::Type::GridMontage: {
    return complex::GridMontage::k_TypeName;
  }
  case complex::DataObject::Type::Any: {
    return {"Any"};
  }
  default: {
    throw std::runtime_error("complex::DataObjectTypeToString: Unknown DataObject::Type");
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
    throw std::runtime_error("complex::ConvertArrayTypeToDataObjectType: Invalid ArrayType");
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
} // namespace complex

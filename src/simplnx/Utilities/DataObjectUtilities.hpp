#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractArray.hpp"
#include "simplnx/DataStructure/AbstractDataObject.hpp"
#include "simplnx/DataStructure/AbstractNeighborList.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGeometry.hpp"
#include "simplnx/DataStructure/Geometry/AbstractGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/Montage/AbstractMontage.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

namespace nx::core
{
/**
 * @brief This method will assign a new 'id' to a value only if the boolean referenced by the visitedIndex is false.
 * @param originalId The original 'id'
 * @param updatedId The updated 'id'
 * @param visited The vector of visited values
 * @param visitedIndex The index into the visited array
 * @return Either the original 'id' if this was already visited or the new 'id' if it was not visited already.
 */
inline constexpr std::optional<AbstractDataObject::IdType> VisitDataStructureId(std::optional<AbstractDataObject::IdType>& originalId,
                                                                                const std::pair<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedId, std::vector<bool>& visited,
                                                                                usize visitedIndex)
{
  if(originalId == updatedId.first && !visited[visitedIndex])
  {
    visited[visitedIndex] = true;
    return updatedId.second;
  }
  return originalId;
}

/**
 * @brief Returns a string representation of the passed in AbstractGeometry::Type
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
    throw std::runtime_error("nx::core::GeometryTypeToString: Unknown AbstractGeometry::Type");
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
 * @brief Returns a AbstractGeometry::Type for the passed in string representation
 * @param geomTypeString
 * @return
 */
inline constexpr AbstractGeometry::Type StringToGeometryType(std::string_view geomTypeString)
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
    throw std::runtime_error("nx::core::StringToGeometryType: No known AbstractGeometry::Type matches the given string value.");
  }
}

inline constexpr StringLiteral DataObjectTypeToString(AbstractDataObject::Type dataObjType)
{
  switch(dataObjType)
  {
  case nx::core::IDataObject::Type::BaseGroup: {
    return nx::core::BaseGroup::k_TypeName;
  }
  case nx::core::IDataObject::Type::DataGroup: {
    return nx::core::DataGroup::k_TypeName;
  }
  case nx::core::IDataObject::Type::AttributeMatrix: {
    return nx::core::AttributeMatrix::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractGeometry: {
    return nx::core::AbstractGeometry::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractGridGeometry: {
    return nx::core::AbstractGridGeometry::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractNodeGeometry0D: {
    return nx::core::AbstractNodeGeometry0D::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractNodeGeometry1D: {
    return nx::core::AbstractNodeGeometry1D::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractNodeGeometry2D: {
    return nx::core::AbstractNodeGeometry2D::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractNodeGeometry3D: {
    return nx::core::AbstractNodeGeometry3D::k_TypeName;
  }
  case nx::core::IDataObject::Type::ImageGeom: {
    return nx::core::ImageGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::RectGridGeom: {
    return nx::core::RectGridGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::VertexGeom: {
    return nx::core::VertexGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::EdgeGeom: {
    return nx::core::EdgeGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::TriangleGeom: {
    return nx::core::TriangleGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::QuadGeom: {
    return nx::core::QuadGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::TetrahedralGeom: {
    return nx::core::TetrahedralGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::HexahedralGeom: {
    return nx::core::HexahedralGeom::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractDataArray: {
    return nx::core::AbstractDataArray::k_TypeName;
  }
  case nx::core::IDataObject::Type::DataArray: {
    return nx::core::DataArrayConstants::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractNeighborList: {
    return nx::core::AbstractNeighborList::k_TypeName;
  }
  case nx::core::IDataObject::Type::NeighborList: {
    return nx::core::NeighborListConstants::k_TypeName;
  }
  case nx::core::IDataObject::Type::ScalarData: {
    return nx::core::ScalarDataConstants::k_TypeName;
  }
  case nx::core::IDataObject::Type::StringArray: {
    return nx::core::StringArray::k_TypeName;
  }
  case nx::core::IDataObject::Type::DynamicListArray: {
    return nx::core::DynamicListArrayConstants::k_TypeName;
  }
  case nx::core::IDataObject::Type::AbstractMontage: {
    return nx::core::AbstractMontage::k_TypeName;
  }
  case nx::core::IDataObject::Type::GridMontage: {
    return nx::core::GridMontage::k_TypeName;
  }
  case nx::core::IDataObject::Type::Any: {
    return {"Any"};
  }
  default: {
    throw std::runtime_error("nx::core::DataObjectTypeToString: Unknown DataObject::Type");
  }
  }
}

/**
 * @brief Converts AbstractArray::ArrayType to AbstractDataObject::Type. ArrayType is a subset of AbstractDataObject::Type so this function cannot fail.
 * @param arrayType
 * @return
 */
inline constexpr AbstractDataObject::Type ConvertArrayTypeToDataObjectType(AbstractArray::ArrayType arrayType)
{
  switch(arrayType)
  {
  case AbstractArray::ArrayType::DataArray: {
    return IDataObject::Type::DataArray;
  }
  case AbstractArray::ArrayType::NeighborListArray: {
    return IDataObject::Type::NeighborList;
  }
  case AbstractArray::ArrayType::StringArray: {
    return IDataObject::Type::StringArray;
  }
  case AbstractArray::ArrayType::Any: {
    return IDataObject::Type::Any;
  }
  default: {
    throw std::runtime_error("nx::core::ConvertArrayTypeToDataObjectType: Invalid ArrayType");
  }
  }
}

/**
 * @brief Converts AbstractDataObject::Type to AbstractArray::ArrayType.
 * @param dataObjectType
 * @return
 */
inline constexpr std::optional<AbstractArray::ArrayType> ConvertDataObjectTypeToArrayType(AbstractDataObject::Type dataObjectType) noexcept
{
  switch(dataObjectType)
  {
  case IDataObject::Type::AbstractDataArray:
  case IDataObject::Type::DataArray: {
    return AbstractArray::ArrayType::DataArray;
  }
  case IDataObject::Type::AbstractNeighborList:
  case IDataObject::Type::NeighborList: {
    return AbstractArray::ArrayType::NeighborListArray;
  }
  case IDataObject::Type::StringArray: {
    return AbstractArray::ArrayType::StringArray;
  }
  case IDataObject::Type::Any: {
    return AbstractArray::ArrayType::Any;
  }
  default: {
    return {};
  }
  }
}
} // namespace nx::core

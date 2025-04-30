#pragma once

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

namespace nx::core::UnitTest::Comparison
{
inline bool CompareDataObject(const DataObject& exemplarObject, const DataObject& testObject);

inline bool CompareBaseGroup(const BaseGroup& exemplarObject, const BaseGroup& testObject)
{
  if(testObject.getSize() != exemplarObject.getSize())
  {
    return false;
  }

  for(auto&& [id, object] : exemplarObject)
  {
    std::string childName = object->getName();
    if(!testObject.contains(childName))
    {
      return false;
    }
    const DataObject* exemplarChild = exemplarObject[childName];
    const DataObject* testChild = testObject[childName];
    // Recursive. Could have issues with a deeply nested DataStructure
    if(!CompareDataObject(*exemplarChild, *testChild))
    {
      return false;
    }
  }

  return true;
}

inline bool CompareIArray(const IArray& exemplarObject, const IArray& testObject)
{
  return testObject.getTupleShape() == exemplarObject.getTupleShape() && testObject.getComponentShape() == exemplarObject.getComponentShape();
}

inline bool CompareINeighborList(const INeighborList& exemplarObject, const INeighborList& testObject)
{
  if(testObject.getDataType() != exemplarObject.getDataType())
  {
    return false;
  }

  return CompareIArray(exemplarObject, testObject);
}

template <class T>
bool CompareNeighborListValues(const NeighborList<T>& exemplarObject, const NeighborList<T>& testObject)
{
  int32 numLists = exemplarObject.getNumberOfLists();
  for(int32 i = 0; i < numLists; i++)
  {
    if(exemplarObject.getList(i) != testObject.getList(i))
    {
      return false;
    }
  }

  return true;
}

struct CompareNeighborListFunctor
{
  template <class T>
  bool operator()(const INeighborList& exemplarObject, const INeighborList& testObject) const
  {
    return CompareNeighborListValues(dynamic_cast<const NeighborList<T>&>(exemplarObject), dynamic_cast<const NeighborList<T>&>(testObject));
  }
};

inline bool CompareNeighborList(const INeighborList& exemplarObject, const INeighborList& testObject)
{
  if(!CompareINeighborList(exemplarObject, testObject))
  {
    return false;
  }

  DataType dataType = exemplarObject.getDataType();

  return ExecuteNeighborFunction(CompareNeighborListFunctor{}, dataType, exemplarObject, testObject);
}

inline bool CompareIDataArray(const IDataArray& exemplarObject, const IDataArray& testObject)
{
  if(testObject.getDataType() != exemplarObject.getDataType())
  {
    return false;
  }

  return CompareIArray(exemplarObject, testObject);
}

template <class T>
bool CompareDataArrayValues(const DataArray<T>& exemplarObject, const DataArray<T>& testObject)
{
  usize size = exemplarObject.size();
  for(usize i = 0; i < size; i++)
  {
    if(testObject[i] != exemplarObject[i])
    {
      return false;
    }
  }

  return true;
}

struct CompareDataArrayFunctor
{
  template <class T>
  bool operator()(const IDataArray& exemplarObject, const IDataArray& testObject) const
  {
    return CompareDataArrayValues(dynamic_cast<const DataArray<T>&>(exemplarObject), dynamic_cast<const DataArray<T>&>(testObject));
  }
};

inline bool CompareDataArray(const IDataArray& exemplarObject, const IDataArray& testObject)
{
  if(!CompareIDataArray(exemplarObject, testObject))
  {
    return false;
  }

  DataType dataType = exemplarObject.getDataType();

  return ExecuteDataFunction(CompareDataArrayFunctor{}, dataType, exemplarObject, testObject);
}

inline bool CompareStringArray(const StringArray& exemplarObject, const StringArray& testObject)
{
  if(!CompareIArray(exemplarObject, testObject))
  {
    return false;
  }

  usize size = exemplarObject.size();
  for(usize i = 0; i < size; i++)
  {
    if(testObject[i] != exemplarObject[i])
    {
      return false;
    }
  }

  return true;
}

inline bool CompareAttributeMatrix(const AttributeMatrix& exemplarObject, const AttributeMatrix& testObject)
{
  if(testObject.getShape() != exemplarObject.getShape())
  {
    return false;
  }

  return CompareBaseGroup(exemplarObject, testObject);
}

inline bool CheckOptionalObject(const DataObject* exemplarObject, const DataObject* testObject)
{
  if((exemplarObject == nullptr && testObject != nullptr) || (exemplarObject != nullptr && testObject == nullptr))
  {
    return false;
  }
  if(exemplarObject != nullptr && testObject != nullptr)
  {
    return exemplarObject->getDataPaths() == testObject->getDataPaths();
  }
  return true;
}

inline bool CompareIGeometry(const IGeometry& exemplarObject, const IGeometry& testObject)
{
  if(exemplarObject.getSpatialDimensionality() != testObject.getSpatialDimensionality())
  {
    return false;
  }
  if(exemplarObject.getUnitDimensionality() != testObject.getUnitDimensionality())
  {
    return false;
  }
  if(exemplarObject.getUnits() != testObject.getUnits())
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getElementSizes(), testObject.getElementSizes()))
  {
    return false;
  }

  return CompareBaseGroup(exemplarObject, testObject);
}

inline bool CompareIGridGeometry(const IGridGeometry& exemplarObject, const IGridGeometry& testObject)
{
  if(testObject.getDimensions() != exemplarObject.getDimensions())
  {
    return false;
  }

  if(!CheckOptionalObject(exemplarObject.getCellData(), testObject.getCellData()))
  {
    return false;
  }

  return CompareIGeometry(exemplarObject, testObject);
}

inline bool CompareImageGeom(const ImageGeom& exemplarObject, const ImageGeom& testObject)
{
  if(testObject.getSpacing() != exemplarObject.getSpacing())
  {
    return false;
  }

  if(testObject.getOrigin() != exemplarObject.getOrigin())
  {
    return false;
  }

  return CompareIGridGeometry(exemplarObject, testObject);
}

inline bool CompareRectGridGeom(const RectGridGeom& exemplarObject, const RectGridGeom& testObject)
{
  if(!CheckOptionalObject(exemplarObject.getXBounds(), testObject.getXBounds()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getYBounds(), testObject.getYBounds()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getZBounds(), testObject.getZBounds()))
  {
    return false;
  }

  return CompareIGridGeometry(exemplarObject, testObject);
}

inline bool CompareINodeGeometry0D(const INodeGeometry0D& exemplarObject, const INodeGeometry0D& testObject)
{
  if(!CheckOptionalObject(exemplarObject.getVertices(), testObject.getVertices()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getVertexAttributeMatrix(), testObject.getVertexAttributeMatrix()))
  {
    return false;
  }
  return CompareIGeometry(exemplarObject, testObject);
}

inline bool CompareVertexGeom(const VertexGeom& exemplarObject, const VertexGeom& testObject)
{
  return CompareINodeGeometry0D(exemplarObject, testObject);
}

inline bool CompareINodeGeometry1D(const INodeGeometry1D& exemplarObject, const INodeGeometry1D& testObject)
{
  if(!CheckOptionalObject(exemplarObject.getEdges(), testObject.getEdges()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getEdgeAttributeMatrix(), testObject.getEdgeAttributeMatrix()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getElementsContainingVert(), testObject.getElementsContainingVert()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getElementNeighbors(), testObject.getElementNeighbors()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getElementCentroids(), testObject.getElementCentroids()))
  {
    return false;
  }

  return CompareINodeGeometry0D(exemplarObject, testObject);
}

inline bool CompareEdgeGeom(const EdgeGeom& exemplarObject, const EdgeGeom& testObject)
{
  return CompareINodeGeometry1D(exemplarObject, testObject);
}

inline bool CompareINodeGeometry2D(const INodeGeometry2D& exemplarObject, const INodeGeometry2D& testObject)
{
  if(!CheckOptionalObject(exemplarObject.getFaces(), testObject.getFaces()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getFaceAttributeMatrix(), testObject.getFaceAttributeMatrix()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getUnsharedEdges(), testObject.getUnsharedEdges()))
  {
    return false;
  }

  return CompareINodeGeometry1D(exemplarObject, testObject);
}

inline bool CompareTriangleGeom(const TriangleGeom& exemplarObject, const TriangleGeom& testObject)
{
  return CompareINodeGeometry2D(exemplarObject, testObject);
}

inline bool CompareQuadGeom(const QuadGeom& exemplarObject, const QuadGeom& testObject)
{
  return CompareINodeGeometry2D(exemplarObject, testObject);
}

inline bool CompareINodeGeometry3D(const INodeGeometry3D& exemplarObject, const INodeGeometry3D& testObject)
{
  if(!CheckOptionalObject(exemplarObject.getPolyhedra(), testObject.getPolyhedra()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getPolyhedraAttributeMatrix(), testObject.getPolyhedraAttributeMatrix()))
  {
    return false;
  }
  if(!CheckOptionalObject(exemplarObject.getUnsharedFaces(), testObject.getUnsharedFaces()))
  {
    return false;
  }

  return CompareINodeGeometry2D(exemplarObject, testObject);
}

inline bool CompareTetrahedralGeom(const TetrahedralGeom& exemplarObject, const TetrahedralGeom& testObject)
{
  return CompareINodeGeometry3D(exemplarObject, testObject);
}

inline bool CompareHexahedralGeom(const HexahedralGeom& exemplarObject, const HexahedralGeom& testObject)
{
  return CompareINodeGeometry3D(exemplarObject, testObject);
}

inline bool CompareDataObject(const DataObject& exemplarObject, const DataObject& testObject)
{
  if(testObject.getName() != exemplarObject.getName())
  {
    return false;
  }

  DataObject::Type testObjectType = testObject.getDataObjectType();

  if(testObjectType != testObject.getDataObjectType())
  {
    return false;
  }

  if(typeid(exemplarObject) != typeid(testObject))
  {
    return false;
  }

  switch(testObjectType)
  {
  case DataObject::Type::AttributeMatrix: {
    return CompareAttributeMatrix(dynamic_cast<const AttributeMatrix&>(exemplarObject), dynamic_cast<const AttributeMatrix&>(testObject));
  }
  case DataObject::Type::DataGroup: {
    return CompareBaseGroup(dynamic_cast<const BaseGroup&>(exemplarObject), dynamic_cast<const BaseGroup&>(testObject));
  }
  case DataObject::Type::DataArray: {
    return CompareDataArray(dynamic_cast<const IDataArray&>(exemplarObject), dynamic_cast<const IDataArray&>(testObject));
  }
  case DataObject::Type::TetrahedralGeom: {
    return CompareTetrahedralGeom(dynamic_cast<const TetrahedralGeom&>(exemplarObject), dynamic_cast<const TetrahedralGeom&>(testObject));
  }
  case DataObject::Type::HexahedralGeom: {
    return CompareHexahedralGeom(dynamic_cast<const HexahedralGeom&>(exemplarObject), dynamic_cast<const HexahedralGeom&>(testObject));
  }
  case DataObject::Type::TriangleGeom: {
    return CompareTriangleGeom(dynamic_cast<const TriangleGeom&>(exemplarObject), dynamic_cast<const TriangleGeom&>(testObject));
  }
  case DataObject::Type::QuadGeom: {
    return CompareQuadGeom(dynamic_cast<const QuadGeom&>(exemplarObject), dynamic_cast<const QuadGeom&>(testObject));
  }
  case DataObject::Type::EdgeGeom: {
    return CompareEdgeGeom(dynamic_cast<const EdgeGeom&>(exemplarObject), dynamic_cast<const EdgeGeom&>(testObject));
  }
  case DataObject::Type::VertexGeom: {
    return CompareVertexGeom(dynamic_cast<const VertexGeom&>(exemplarObject), dynamic_cast<const VertexGeom&>(testObject));
  }
  case DataObject::Type::ImageGeom: {
    return CompareImageGeom(dynamic_cast<const ImageGeom&>(exemplarObject), dynamic_cast<const ImageGeom&>(testObject));
  }
  case DataObject::Type::RectGridGeom: {
    return CompareRectGridGeom(dynamic_cast<const RectGridGeom&>(exemplarObject), dynamic_cast<const RectGridGeom&>(testObject));
  }
  case DataObject::Type::StringArray: {
    return CompareStringArray(dynamic_cast<const StringArray&>(exemplarObject), dynamic_cast<const StringArray&>(testObject));
  }
  case DataObject::Type::NeighborList: {
    return CompareNeighborList(dynamic_cast<const INeighborList&>(exemplarObject), dynamic_cast<const INeighborList&>(testObject));
  }
  default: {
    throw std::runtime_error(fmt::format("Equality not implemented for {}", testObject.getTypeName()));
  }
  }
}
} // namespace nx::core::UnitTest::Comparison

#include "EdgeGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name)
: INodeGeometry1D(ds, std::move(name))
{
}

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry1D(ds, std::move(name), importId)
{
}

DataObject::Type EdgeGeom::getDataObjectType() const
{
  return DataObject::Type::EdgeGeom;
}

EdgeGeom* EdgeGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

EdgeGeom* EdgeGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type EdgeGeom::getGeomType() const
{
  return IGeometry::Type::Edge;
}

BaseGroup::GroupType EdgeGeom::getGroupType() const
{
  return GroupType::EdgeGeom;
}

std::string EdgeGeom::getTypeName() const
{
  return "EdgeGeom";
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

std::shared_ptr<DataObject> EdgeGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = *getDataStructure();
  auto copy = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStruct, copyPath.getTargetName(), getId()));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    if(m_VertexAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getVertexAttributeMatrix()))
      {
        const auto dataObjCopy = getVertexAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_VertexDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertices()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(isParentOf(getVertices()))
      {
        const auto dataObjCopy = getVertices()->deepCopy(copiedDataPath);
      }
      copy->m_VertexDataArrayId = dataStruct.getId(copiedDataPath);
    }

    if(m_EdgeAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getEdgeAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getEdgeAttributeMatrix()))
      {
        const auto dataObjCopy = getEdgeAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_EdgeAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_EdgeDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getEdges()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(isParentOf(getEdges()))
      {
        const auto dataObjCopy = getEdges()->deepCopy(copiedDataPath);
      }
      copy->m_EdgeDataArrayId = dataStruct.getId(copiedDataPath);
    }

    if(getElementSizes() != nullptr)
    {
      copy->findElementSizes();
    }
    if(getElementsContainingVert() != nullptr)
    {
      copy->findElementsContainingVert();
    }
    if(getElementNeighbors() != nullptr)
    {
      copy->findElementNeighbors();
    }
    if(getElementCentroids() != nullptr)
    {
      copy->findElementCentroids();
    }
  }

  return copy;
}

IGeometry::StatusCode EdgeGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
  auto* sizes = DataArray<float32>::Create(*getDataStructure(), "Edge Lengths", std::move(dataStore), getId());
  if(sizes == nullptr)
  {
    return -1;
  }
  m_ElementSizesId = sizes->getId();

  std::array<Point3Df, 2> verts = {Point3Df(0.0f, 0.0f, 0.0f), Point3Df(0.0f, 0.0f, 0.0f)};

  for(usize i = 0; i < INodeGeometry1D::getNumberOfCells(); i++)
  {
    getEdgeCoordinates(i, verts);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (verts[0][j] - verts[1][j]) * (verts[0][j] - verts[1][j]);
    }
    (*sizes)[i] = std::sqrt(length);
  }

  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementsContainingVert()
{
  auto* containsVert = ElementDynamicList::Create(*getDataStructure(), "Edges Containing Vert", getId());
  if(containsVert == nullptr)
  {
    return -1;
  }
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), containsVert, getNumberOfVertices());
  if(containsVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  m_CellContainingVertDataArrayId = containsVert->getId();
  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementNeighbors()
{
  StatusCode err = 0;
  if(getElementsContainingVert() == nullptr)
  {
    err = findElementsContainingVert();
    if(err < 0)
    {
      return err;
    }
  }
  auto* edgeNeighbors = ElementDynamicList::Create(*getDataStructure(), "Edge Neighbors", getId());
  if(edgeNeighbors == nullptr)
  {
    err = -1;
    return err;
  }
  m_CellNeighborsDataArrayId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, IGeometry::Type::Edge);
  if(getElementNeighbors() == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    err = -1;
  }
  return err;
}

IGeometry::StatusCode EdgeGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
  auto* edgeCentroids = DataArray<float32>::Create(*getDataStructure(), "Edge Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  if(getElementCentroids() == nullptr)
  {
    return -1;
  }
  m_CellCentroidsDataArrayId = edgeCentroids->getId();
  return 1;
}

Point3D<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = -1.0;
  shape[1] = 1.0;
}

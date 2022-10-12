#include "TriangleGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name)
: INodeGeometry2D(ds, std::move(name))
{
}

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry2D(ds, std::move(name), importId)
{
}

IGeometry::Type TriangleGeom::getGeomType() const
{
  return IGeometry::Type::Triangle;
}

DataObject::Type TriangleGeom::getDataObjectType() const
{
  return DataObject::Type::TriangleGeom;
}

TriangleGeom* TriangleGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TriangleGeom* TriangleGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TriangleGeom::getTypeName() const
{
  return "TriangleGeom";
}

DataObject* TriangleGeom::shallowCopy()
{
  return new TriangleGeom(*this);
}

std::shared_ptr<DataObject> TriangleGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = *getDataStructure();
  auto copy = std::shared_ptr<TriangleGeom>(new TriangleGeom(dataStruct, copyPath.getTargetName(), getId()));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);
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
  if(getUnsharedEdges() != nullptr)
  {
    copy->findUnsharedEdges();
  }
  const DataPath copiedVertDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
  copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedVertDataPath);
  const DataPath copiedVertListPath = copyPath.createChildPath(getVertices()->getName());
  copy->m_VertexDataArrayId = dataStruct.getId(copiedVertListPath);
  const DataPath copiedEdgeDataPath = copyPath.createChildPath(getEdgeAttributeMatrix()->getName());
  copy->m_EdgeAttributeMatrixId = dataStruct.getId(copiedEdgeDataPath);
  const DataPath copiedEdgeListPath = copyPath.createChildPath(getEdges()->getName());
  copy->m_EdgeDataArrayId = dataStruct.getId(copiedEdgeListPath);
  const DataPath copiedFaceDataPath = copyPath.createChildPath(getFaceAttributeMatrix()->getName());
  copy->m_FaceDataId = dataStruct.getId(copiedFaceDataPath);
  const DataPath copiedFaceListPath = copyPath.createChildPath(getFaces()->getName());
  copy->m_FaceListId = dataStruct.getId(copiedFaceListPath);

  return copy;
}

usize TriangleGeom::getNumberOfCells() const
{
  return getFacesRef().getNumberOfTuples();
}

usize TriangleGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

IGeometry::StatusCode TriangleGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{1}, 0.0f);
  Float32Array* triangleSizes = DataArray<float32>::Create(*getDataStructure(), "Triangle Areas", std::move(dataStore), getId());
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), triangleSizes);
  if(triangleSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = triangleSizes->getId();
  return 1;
}

IGeometry::StatusCode TriangleGeom::findElementsContainingVert()
{
  auto trianglesContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangles Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), trianglesContainingVert, getNumberOfVertices());
  if(trianglesContainingVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  m_CellContainingVertDataArrayId = trianglesContainingVert->getId();
  return 1;
}

IGeometry::StatusCode TriangleGeom::findElementNeighbors()
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
  auto triangleNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangle Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), triangleNeighbors, IGeometry::Type::Triangle);
  if(triangleNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  m_CellNeighborsDataArrayId = triangleNeighbors->getId();
  return err;
}

IGeometry::StatusCode TriangleGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{3}, 0.0f);
  auto triangleCentroids = DataArray<float32>::Create(*getDataStructure(), "Triangle Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), triangleCentroids);
  if(triangleCentroids == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  m_CellCentroidsDataArrayId = triangleCentroids->getId();
  return 1;
}

complex::Point3D<float64> TriangleGeom::getParametricCenter() const
{
  return {1.0 / 3.0, 1.0 / 3.0, 0.0};
}

void TriangleGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  // r derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;

  // s derivatives
  shape[3] = -1.0;
  shape[4] = 0.0;
  shape[5] = 1.0;
}

IGeometry::StatusCode TriangleGeom::findEdges()
{
  auto dataStore = std::make_unique<DataStore<uint64>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  DataArray<uint64>* edgeList = DataArray<uint64>::Create(*getDataStructure(), "Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeDataArrayId.reset();
    return -1;
  }
  m_EdgeDataArrayId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode TriangleGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

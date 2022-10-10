#include "QuadGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

QuadGeom::QuadGeom(DataStructure& ds, std::string name)
: INodeGeometry2D(ds, std::move(name))
{
}

QuadGeom::QuadGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry2D(ds, std::move(name), importId)
{
}

IGeometry::Type QuadGeom::getGeomType() const
{
  return IGeometry::Type::Quad;
}

DataObject::Type QuadGeom::getDataObjectType() const
{
  return DataObject::Type::QuadGeom;
}

QuadGeom* QuadGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

QuadGeom* QuadGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string QuadGeom::getTypeName() const
{
  return "QuadGeom";
}

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

DataObject* QuadGeom::deepCopy()
{
  auto dataStruct = *getDataStructure();
  auto* copy = new QuadGeom(dataStruct, getName());
  if(getElementSizes() != nullptr)
  {
    copy->m_ElementSizesId = m_ElementSizesId;
  }
  if(getVertexAttributeMatrix() != nullptr)
  {
    copy->m_VertexAttributeMatrixId = m_VertexAttributeMatrixId;
  }
  if(getVertices() != nullptr)
  {
    copy->m_VertexDataArrayId = m_VertexDataArrayId;
  }
  if(getEdgeAttributeMatrix() != nullptr)
  {
    copy->m_EdgeAttributeMatrixId = m_EdgeAttributeMatrixId;
  }
  if(getEdges() != nullptr)
  {
    copy->m_EdgeDataArrayId = m_EdgeDataArrayId;
  }
  if(getElementsContainingVert() != nullptr)
  {
    copy->m_CellContainingVertDataArrayId = m_CellContainingVertDataArrayId;
  }
  if(getElementNeighbors() != nullptr)
  {
    copy->m_CellNeighborsDataArrayId = m_CellNeighborsDataArrayId;
  }
  if(getElementCentroids() != nullptr)
  {
    copy->m_CellCentroidsDataArrayId = m_CellCentroidsDataArrayId;
  }
  if(getFaceAttributeMatrix() != nullptr)
  {
    copy->m_FaceDataId = m_FaceDataId;
  }
  if(getFaces() != nullptr)
  {
    copy->m_FaceListId = m_FaceListId;
  }
  if(getUnsharedEdges() != nullptr)
  {
    copy->m_UnsharedEdgeListId = m_UnsharedEdgeListId;
  }
  return copy;
}

usize QuadGeom::getNumberOfCells() const
{
  auto& quads = getFacesRef();
  return quads.getNumberOfTuples();
}

usize QuadGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

IGeometry::StatusCode QuadGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
  Float32Array* quadSizes = DataArray<float32>::Create(*getDataStructure(), "Quad Areas", std::move(dataStore), getId());
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), quadSizes);
  if(quadSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = quadSizes->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementsContainingVert()
{
  auto quadsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quads Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), quadsContainingVert, getNumberOfVertices());
  if(quadsContainingVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  m_CellContainingVertDataArrayId = quadsContainingVert->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementNeighbors()
{
  if(getElementsContainingVert() == nullptr)
  {
    StatusCode err = findElementsContainingVert();
    if(err < 0)
    {
      return err;
    }
  }
  auto quadNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quad Neighbors", getId());
  StatusCode err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), quadNeighbors, IGeometry::Type::Quad);
  if(quadNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  m_CellNeighborsDataArrayId = quadNeighbors->getId();
  return err;
}

IGeometry::StatusCode QuadGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
  auto quadCentroids = DataArray<float32>::Create(*getDataStructure(), "Quad Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), quadCentroids);
  if(quadCentroids == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  m_CellCentroidsDataArrayId = quadCentroids->getId();
  return 1;
}

Point3D<float64> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
{
  float64 rm = 1.0 - pCoords[0];
  float64 sm = 1.0 - pCoords[1];

  shape[0] = -sm;
  shape[1] = sm;
  shape[2] = pCoords[1];
  shape[3] = -pCoords[1];
  shape[4] = -rm;
  shape[5] = -pCoords[0];
  shape[6] = pCoords[0];
  shape[7] = rm;
}

IGeometry::StatusCode QuadGeom::findEdges()
{
  auto* edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeDataArrayId.reset();
    return -1;
  }
  m_EdgeDataArrayId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

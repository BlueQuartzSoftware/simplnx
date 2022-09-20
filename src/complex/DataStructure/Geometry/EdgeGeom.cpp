#include "EdgeGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include "FileVec/collection/IGroup.hpp"

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

std::string EdgeGeom::getTypeName() const
{
  return "EdgeGeom";
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

DataObject* EdgeGeom::deepCopy()
{
  return new EdgeGeom(*this);
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
    m_CellContainingVertId.reset();
    return -1;
  }
  m_CellContainingVertId = containsVert->getId();
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
  m_CellNeighborsId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, IGeometry::Type::Edge);
  if(getElementNeighbors() == nullptr)
  {
    m_CellNeighborsId.reset();
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
  m_CellCentroidsId = edgeCentroids->getId();
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

#if 0
Zarr::ErrorType EdgeGeom::readZarr(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& collection, bool preflight)
{
  m_VertexListId = ReadZarrDataId(collection, H5Constants::k_VertexListTag);
  m_EdgeListId = ReadZarrDataId(collection, H5Constants::k_EdgeListTag);
  m_EdgesContainingVertId = ReadZarrDataId(collection, H5Constants::k_EdgesContainingVertTag);
  m_EdgeNeighborsId = ReadZarrDataId(collection, H5Constants::k_EdgeNeighborsTag);
  m_EdgeCentroidsId = ReadZarrDataId(collection, H5Constants::k_EdgeCentroidTag);
  m_EdgeSizesId = ReadZarrDataId(collection, H5Constants::k_EdgeSizesTag);

  return BaseGroup::readZarr(dataStructureReader, collection, preflight);
}

Zarr::ErrorType EdgeGeom::writeZarr(Zarr::DataStructureWriter& dataStructureWriter, FileVec::IGroup& parentGroupWriter, bool importable) const
{
  auto groupWriterPtr = parentGroupWriter.createOrFindGroup(getName());
  auto& groupWriter = *groupWriterPtr.get();

  writeZarrObjectAttributes(dataStructureWriter, groupWriter, importable);

  WriteZarrDataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  WriteZarrDataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  WriteZarrDataId(groupWriter, m_EdgesContainingVertId, H5Constants::k_EdgesContainingVertTag);
  WriteZarrDataId(groupWriter, m_EdgeNeighborsId, H5Constants::k_EdgeNeighborsTag);
  WriteZarrDataId(groupWriter, m_EdgeCentroidsId, H5Constants::k_EdgeCentroidTag);
  WriteZarrDataId(groupWriter, m_EdgeSizesId, H5Constants::k_EdgeSizesTag);

  return getDataMap().writeZarrGroup(dataStructureWriter, groupWriter);
}
#endif

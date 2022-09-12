#include "INodeGeometry3D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
{
INodeGeometry3D::INodeGeometry3D(DataStructure& ds, std::string name)
: INodeGeometry2D(ds, std::move(name))
{
}

INodeGeometry3D::INodeGeometry3D(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry2D(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry3D::IdType>& INodeGeometry3D::getPolyhedronListId() const
{
  return m_PolyhedronListId;
}

INodeGeometry3D::SharedFaceList* INodeGeometry3D::getPolyhedra()
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

const INodeGeometry3D::SharedFaceList* INodeGeometry3D::getPolyhedra() const
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

INodeGeometry3D::SharedFaceList& INodeGeometry3D::getPolyhedraRef()
{
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
}

const INodeGeometry3D::SharedFaceList& INodeGeometry3D::getPolyhedraRef() const
{
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
}

void INodeGeometry3D::setPolyhedra(const SharedFaceList& polyhedra)
{
  m_PolyhedronListId = polyhedra.getId();
}

void INodeGeometry3D::resizePolyhedronList(usize size)
{
  getPolyhedraRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry3D::getNumberOfPolyhedra() const
{
  const auto& polyhedra = getPolyhedraRef();
  return polyhedra.getNumberOfTuples();
}

void INodeGeometry3D::deleteFaces()
{
  getDataStructureRef().removeData(m_FaceListId);
  m_FaceListId.reset();
}

const std::optional<INodeGeometry3D::IdType>& INodeGeometry3D::getUnsharedFacesId() const
{
  return m_UnsharedFaceListId;
}

const INodeGeometry3D::SharedFaceList* INodeGeometry3D::getUnsharedFaces() const
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_UnsharedFaceListId);
}

void INodeGeometry3D::deleteUnsharedFaces()
{
  getDataStructureRef().removeData(m_UnsharedFaceListId);
  m_UnsharedFaceListId.reset();
}

void INodeGeometry3D::setCoords(usize vertId, const Point3D<float32>& coords)
{
  auto& vertices = getVerticesRef();

  usize index = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    vertices[index + i] = coords[i];
  }
}

Point3D<float32> INodeGeometry3D::getCoords(usize vertId) const
{
  auto& vertices = getVerticesRef();

  usize index = vertId * 3;
  auto x = vertices[index];
  auto y = vertices[index + 1];
  auto z = vertices[index + 2];
  return Point3D<float32>(x, y, z);
}

void INodeGeometry3D::getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const
{
  auto& vertices = getVerticesRef();

  usize verts[2];
  getVertsAtEdge(edgeId, verts);

  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = vertices[verts[0] * 3 + i];
    vert2[i] = vertices[verts[1] * 3 + i];
  }
}

const std::optional<INodeGeometry3D::IdType>& INodeGeometry3D::getPolyhedraDataId() const
{
  return m_PolyhedronDataId;
}

AttributeMatrix* INodeGeometry3D::getPolyhedronData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronDataId);
}

const AttributeMatrix* INodeGeometry3D::getPolyhedronData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronDataId);
}

AttributeMatrix& INodeGeometry3D::getPolyhedronDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronDataId.value());
}

const AttributeMatrix& INodeGeometry3D::getPolyhedronDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronDataId.value());
}

DataPath INodeGeometry3D::getPolyhedronDataPath() const
{
  return getPolyhedronDataRef().getDataPaths().at(0);
}

void INodeGeometry3D::setPolyhedronData(const AttributeMatrix& attributeMatrix)
{
  m_PolyhedronDataId = attributeMatrix.getId();
}

H5::ErrorType INodeGeometry3D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = INodeGeometry2D::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_PolyhedronListId = ReadH5DataId(groupReader, H5Constants::k_PolyhedronListTag);
  m_PolyhedronDataId = ReadH5DataId(groupReader, H5Constants::k_PolyhedronDataTag);
  m_UnsharedFaceListId = ReadH5DataId(groupReader, H5Constants::k_UnsharedFaceListTag);

  return error;
}

H5::ErrorType INodeGeometry3D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = INodeGeometry2D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_PolyhedronListId, H5Constants::k_PolyhedronListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_PolyhedronDataId, H5Constants::k_PolyhedronDataTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_UnsharedFaceListId, H5Constants::k_UnsharedFaceListTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}

INodeGeometry3D::SharedQuadList* INodeGeometry3D::createSharedQuadList(usize numQuads)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numQuads}, std::vector<usize>{4}, 0);
  SharedQuadList* quads = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Quad List", std::move(dataStore), getId());
  return quads;
}

INodeGeometry3D::SharedTriList* INodeGeometry3D::createSharedTriList(usize numTris)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numTris}, std::vector<usize>{3}, 0);
  SharedTriList* triangles = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Tri List", std::move(dataStore), getId());
  return triangles;
}

void INodeGeometry3D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry2D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_PolyhedronListId == updatedId.first)
    {
      m_PolyhedronListId = updatedId.second;
    }

    if(m_PolyhedronDataId == updatedId.first)
    {
      m_PolyhedronDataId = updatedId.second;
    }

    if(m_UnsharedFaceListId == updatedId.first)
    {
      m_UnsharedFaceListId = updatedId.second;
    }
  }
}
} // namespace complex

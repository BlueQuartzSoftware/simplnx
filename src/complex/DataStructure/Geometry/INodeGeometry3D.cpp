#include "INodeGeometry3D.hpp"

namespace complex
{
INodeGeometry3D::INodeGeometry3D(DataStructure& dataStructure, std::string name)
: INodeGeometry2D(dataStructure, std::move(name))
{
}

INodeGeometry3D::INodeGeometry3D(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry2D(dataStructure, std::move(name), importId)
{
}

const std::optional<INodeGeometry3D::IdType>& INodeGeometry3D::getPolyhedronListId() const
{
  return m_PolyhedronListId;
}

void INodeGeometry3D::setPolyhedronListId(const OptionalId& polyListId)
{
  m_PolyhedronListId = polyListId;
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

INodeGeometry3D::OptionalId INodeGeometry3D::getPolyhedraDataId() const
{
  return m_PolyhedronAttributeMatrixId;
}

void INodeGeometry3D::setPolyhedraList(const SharedFaceList& polyhedra)
{
  m_PolyhedronListId = polyhedra.getId();
}

void INodeGeometry3D::resizePolyhedraList(usize size)
{
  getPolyhedraRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry3D::getNumberOfPolyhedra() const
{
  const auto& polyhedra = getPolyhedraRef();
  return polyhedra.getNumberOfTuples();
}

void INodeGeometry3D::setCellPointIds(usize polyhedraId, nonstd::span<usize> vertexIds)
{
  auto& polyhedra = getPolyhedraRef();
  usize numVerts = getNumberOfVerticesPerCell();
  const usize offset = polyhedraId * numVerts;
  if(offset + numVerts > polyhedra.getSize())
  {
    return;
  }
  for(usize i = 0; i < numVerts; i++)
  {
    polyhedra[polyhedraId * numVerts + i] = vertexIds[i];
  }
}

void INodeGeometry3D::getCellPointIds(usize polyhedraId, nonstd::span<usize> vertexIds) const
{
  auto& polyhedra = getPolyhedraRef();
  usize numVerts = getNumberOfVerticesPerCell();
  const usize offset = polyhedraId * numVerts;
  if(offset + numVerts > polyhedra.getSize())
  {
    return;
  }
  for(usize i = 0; i < numVerts; i++)
  {
    vertexIds[i] = polyhedra[offset + i];
  }
}

void INodeGeometry3D::getCellCoordinates(usize hexId, nonstd::span<Point3Df> coords) const
{
  usize numVerts = getNumberOfVerticesPerCell();
  std::vector<usize> vertIds(numVerts, 0);
  getCellPointIds(hexId, vertIds);
  for(usize index = 0; index < numVerts; index++)
  {
    coords[index] = getVertexCoordinate(vertIds[index]);
  }
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

void INodeGeometry3D::setUnsharedFacedId(const OptionalId& id)
{
  m_UnsharedFaceListId = id;
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

const std::optional<INodeGeometry3D::IdType>& INodeGeometry3D::getPolyhedraAttributeMatrixId() const
{
  return m_PolyhedronAttributeMatrixId;
}

void INodeGeometry3D::setPolyhedraDataId(const OptionalId& polyDataId)
{
  m_PolyhedronAttributeMatrixId = polyDataId;
}

AttributeMatrix* INodeGeometry3D::getPolyhedraAttributeMatrix()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry3D::getPolyhedraAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

AttributeMatrix& INodeGeometry3D::getPolyhedraAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry3D::getPolyhedraAttributeMatrixRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId.value());
}

DataPath INodeGeometry3D::getPolyhedronAttributeMatrixDataPath() const
{
  return getPolyhedraAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry3D::setPolyhedraAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_PolyhedronAttributeMatrixId = attributeMatrix.getId();
}
INodeGeometry3D::SharedQuadList* INodeGeometry3D::createSharedQuadList(usize numQuads)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numQuads}, std::vector<usize>{4}, 0);
  SharedQuadList* quads = DataArray<MeshIndexType>::Create(*getDataStructure(), k_QuadFaceList, std::move(dataStore), getId());
  return quads;
}

INodeGeometry3D::SharedTriList* INodeGeometry3D::createSharedTriList(usize numTris)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numTris}, std::vector<usize>{3}, 0);
  SharedTriList* triangles = DataArray<MeshIndexType>::Create(*getDataStructure(), k_TriangleFaceList, std::move(dataStore), getId());
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

    if(m_PolyhedronAttributeMatrixId == updatedId.first)
    {
      m_PolyhedronAttributeMatrixId = updatedId.second;
    }

    if(m_UnsharedFaceListId == updatedId.first)
    {
      m_UnsharedFaceListId = updatedId.second;
    }
  }
}
} // namespace complex

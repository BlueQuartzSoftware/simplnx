#include "INodeGeometry3D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
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
  if(!m_PolyhedronListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

const INodeGeometry3D::SharedFaceList* INodeGeometry3D::getPolyhedra() const
{
  if(!m_PolyhedronListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

INodeGeometry3D::SharedFaceList& INodeGeometry3D::getPolyhedraRef()
{
  if(!m_PolyhedronListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared polyhedra list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
}

const INodeGeometry3D::SharedFaceList& INodeGeometry3D::getPolyhedraRef() const
{
  if(!m_PolyhedronListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared polyhedra list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
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
  getPolyhedraRef().getIDataStoreRef().resizeTuples({size});
}

usize INodeGeometry3D::getNumberOfPolyhedra() const
{
  const auto* polyhedraPtr = getPolyhedra();
  return polyhedraPtr == nullptr ? 0 : polyhedraPtr->getNumberOfTuples();
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
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry3D::getPolyhedraAttributeMatrix() const
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

AttributeMatrix& INodeGeometry3D::getPolyhedraAttributeMatrixRef()
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a polyhedra attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry3D::getPolyhedraAttributeMatrixRef() const
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a polyhedra attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
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
  std::vector<bool> visited(3, false);

  for(const auto& updatedId : updatedIds)
  {
    m_PolyhedronListId = nx::core::VisitDataStructureId(m_PolyhedronListId, updatedId, visited, 0);
    m_PolyhedronAttributeMatrixId = nx::core::VisitDataStructureId(m_PolyhedronAttributeMatrixId, updatedId, visited, 1);
    m_UnsharedFaceListId = nx::core::VisitDataStructureId(m_UnsharedFaceListId, updatedId, visited, 2);
  }
}

Result<> INodeGeometry3D::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result = INodeGeometry2D::validate();

  usize numTuples = getNumberOfPolyhedra();
  const AttributeMatrix* amPtr = getPolyhedraAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Hex/Tet Geometry '{}' has {} cells but the cell Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}
} // namespace nx::core

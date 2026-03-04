#include "AbstractNodeGeometry3D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
AbstractNodeGeometry3D::AbstractNodeGeometry3D(DataStructure& dataStructure, std::string name)
: AbstractNodeGeometry2D(dataStructure, std::move(name))
{
}

AbstractNodeGeometry3D::AbstractNodeGeometry3D(DataStructure& dataStructure, std::string name, IdType importId)
: AbstractNodeGeometry2D(dataStructure, std::move(name), importId)
{
}

const std::optional<AbstractNodeGeometry3D::IdType>& AbstractNodeGeometry3D::getPolyhedronListId() const
{
  return m_PolyhedronListId;
}

void AbstractNodeGeometry3D::setPolyhedronListId(const OptionalId& polyListId)
{
  m_PolyhedronListId = polyListId;
}

AbstractNodeGeometry3D::SharedFaceList* AbstractNodeGeometry3D::getPolyhedra()
{
  if(!m_PolyhedronListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

const AbstractNodeGeometry3D::SharedFaceList* AbstractNodeGeometry3D::getPolyhedra() const
{
  if(!m_PolyhedronListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_PolyhedronListId);
}

AbstractNodeGeometry3D::SharedFaceList& AbstractNodeGeometry3D::getPolyhedraRef()
{
  if(!m_PolyhedronListId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared polyhedra list assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
}

const AbstractNodeGeometry3D::SharedFaceList& AbstractNodeGeometry3D::getPolyhedraRef() const
{
  if(!m_PolyhedronListId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared polyhedra list assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_PolyhedronListId.value());
}

AbstractNodeGeometry3D::OptionalId AbstractNodeGeometry3D::getPolyhedraDataId() const
{
  return m_PolyhedronAttributeMatrixId;
}

void AbstractNodeGeometry3D::setPolyhedraList(const SharedFaceList& polyhedra)
{
  m_PolyhedronListId = polyhedra.getId();
}

void AbstractNodeGeometry3D::resizePolyhedraList(usize size)
{
  getPolyhedraRef().getIDataStoreRef().resizeTuples({size});
}

usize AbstractNodeGeometry3D::getNumberOfPolyhedra() const
{
  const auto* polyhedraPtr = getPolyhedra();
  return polyhedraPtr == nullptr ? 0 : polyhedraPtr->getNumberOfTuples();
}

void AbstractNodeGeometry3D::setCellPointIds(usize polyhedraId, nonstd::span<usize> vertexIds)
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

void AbstractNodeGeometry3D::getCellPointIds(usize polyhedraId, nonstd::span<usize> vertexIds) const
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

void AbstractNodeGeometry3D::getCellCoordinates(usize hexId, nonstd::span<Point3Df> coords) const
{
  usize numVerts = getNumberOfVerticesPerCell();
  std::vector<usize> vertIds(numVerts, 0);
  getCellPointIds(hexId, vertIds);
  for(usize index = 0; index < numVerts; index++)
  {
    coords[index] = getVertexCoordinate(vertIds[index]);
  }
}

void AbstractNodeGeometry3D::deleteFaces()
{
  getDataStructureRef().removeData(m_FaceListId);
  m_FaceListId.reset();
}

const std::optional<AbstractNodeGeometry3D::IdType>& AbstractNodeGeometry3D::getUnsharedFacesId() const
{
  return m_UnsharedFaceListId;
}

void AbstractNodeGeometry3D::setUnsharedFacedId(const OptionalId& id)
{
  m_UnsharedFaceListId = id;
}

const AbstractNodeGeometry3D::SharedFaceList* AbstractNodeGeometry3D::getUnsharedFaces() const
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_UnsharedFaceListId);
}

void AbstractNodeGeometry3D::deleteUnsharedFaces()
{
  getDataStructureRef().removeData(m_UnsharedFaceListId);
  m_UnsharedFaceListId.reset();
}

const std::optional<AbstractNodeGeometry3D::IdType>& AbstractNodeGeometry3D::getPolyhedraAttributeMatrixId() const
{
  return m_PolyhedronAttributeMatrixId;
}

void AbstractNodeGeometry3D::setPolyhedraDataId(const OptionalId& polyDataId)
{
  m_PolyhedronAttributeMatrixId = polyDataId;
}

AttributeMatrix* AbstractNodeGeometry3D::getPolyhedraAttributeMatrix()
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

const AttributeMatrix* AbstractNodeGeometry3D::getPolyhedraAttributeMatrix() const
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId);
}

AttributeMatrix& AbstractNodeGeometry3D::getPolyhedraAttributeMatrixRef()
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a polyhedra attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId.value());
}

const AttributeMatrix& AbstractNodeGeometry3D::getPolyhedraAttributeMatrixRef() const
{
  if(!m_PolyhedronAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a polyhedra attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_PolyhedronAttributeMatrixId.value());
}

DataPath AbstractNodeGeometry3D::getPolyhedronAttributeMatrixDataPath() const
{
  return getPolyhedraAttributeMatrixRef().getDataPaths().at(0);
}

void AbstractNodeGeometry3D::setPolyhedraAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_PolyhedronAttributeMatrixId = attributeMatrix.getId();
}
AbstractNodeGeometry3D::SharedQuadList* AbstractNodeGeometry3D::createSharedQuadList(usize numQuads)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numQuads}, std::vector<usize>{4}, 0);
  SharedQuadList* quads = DataArray<MeshIndexType>::Create(*getDataStructure(), k_SharedFacesListName, std::move(dataStore), getId());
  return quads;
}

AbstractNodeGeometry3D::SharedTriList* AbstractNodeGeometry3D::createSharedTriList(usize numTris)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numTris}, std::vector<usize>{3}, 0);
  SharedTriList* triangles = DataArray<MeshIndexType>::Create(*getDataStructure(), k_SharedFacesListName, std::move(dataStore), getId());
  return triangles;
}

void AbstractNodeGeometry3D::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  AbstractNodeGeometry2D::checkUpdatedIdsImpl(updatedIdsMap);
  std::vector<bool> visited(3, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_PolyhedronListId = nx::core::VisitDataStructureId(m_PolyhedronListId, updatedId, visited, 0);
    m_PolyhedronAttributeMatrixId = nx::core::VisitDataStructureId(m_PolyhedronAttributeMatrixId, updatedId, visited, 1);
    m_UnsharedFaceListId = nx::core::VisitDataStructureId(m_UnsharedFaceListId, updatedId, visited, 2);
  }
}

Result<> AbstractNodeGeometry3D::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result = AbstractNodeGeometry2D::validate();

  usize numTuples = getNumberOfPolyhedra();
  const AttributeMatrix* amPtr = getPolyhedraAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumberOfTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Hex/Tet Geometry '{}' has {} cells but the cell Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}
} // namespace nx::core

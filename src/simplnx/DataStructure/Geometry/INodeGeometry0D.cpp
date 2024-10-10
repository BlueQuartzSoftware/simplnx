#include "INodeGeometry0D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
INodeGeometry0D::INodeGeometry0D(DataStructure& dataStructure, std::string name)
: IGeometry(dataStructure, std::move(name))
{
}

INodeGeometry0D::INodeGeometry0D(DataStructure& dataStructure, std::string name, IdType importId)
: IGeometry(dataStructure, std::move(name), importId)
{
}

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getSharedVertexDataArrayId() const
{
  return m_VertexDataArrayId;
}

INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices()
{
  if(!m_VertexDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

const INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices() const
{
  if(!m_VertexDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef()
{
  if(!m_VertexDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry0D::{} threw runtime exception. The geometry with name '{}' does not have a shared vertex list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

const INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef() const
{
  if(!m_VertexDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry0D::{} threw runtime exception. The geometry with name '{}' does not have a shared vertex list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

void INodeGeometry0D::setVertices(const INodeGeometry0D::SharedVertexList& vertices)
{
  m_VertexDataArrayId = vertices.getId();
}

std::optional<DataObject::IdType> INodeGeometry0D::getVertexListId() const
{
  return m_VertexDataArrayId;
}

void INodeGeometry0D::setVertexListId(const std::optional<IdType>& vertices)
{
  m_VertexDataArrayId = vertices;
}

void INodeGeometry0D::resizeVertexList(usize size)
{
  getVerticesRef().getIDataStoreRef().resizeTuples({size});
}

usize INodeGeometry0D::getNumberOfVertices() const
{
  const auto* verticesPtr = getVertices();
  return verticesPtr == nullptr ? 0 : verticesPtr->getNumberOfTuples();
}

usize INodeGeometry0D::getNumberOfCells() const
{
  return getNumberOfVertices();
}

BoundingBox3Df INodeGeometry0D::getBoundingBox() const
{
  FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

  const IGeometry::SharedVertexList& vertexList = getVerticesRef();
  if(vertexList.getDataType() != DataType::float32)
  {
    return {ll, ur}; // will be invalid
  }

  try
  {
    auto& vertexListStore = vertexList.template getIDataStoreRefAs<const DataStore<float32>>();

    for(size_t tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
    {
      float x = vertexListStore.getComponentValue(tuple, 0);
      ll[0] = (x < ll[0]) ? x : ll[0];
      ur[0] = (x > ur[0]) ? x : ur[0];

      float y = vertexListStore.getComponentValue(tuple, 1);
      ll[1] = (y < ll[1]) ? y : ll[1];
      ur[1] = (y > ur[1]) ? y : ur[1];

      float z = vertexListStore.getComponentValue(tuple, 2);
      ll[2] = (z < ll[2]) ? z : ll[2];
      ur[2] = (z > ur[2]) ? z : ur[2];
    }
  } catch(std::bad_cast ex)
  {
    return {ll, ur}; // will be invalid
  }

  return {ll, ur}; // should be valid
}

Result<bool> INodeGeometry0D::isPlane(usize dimensionIndex) const
{
  try
  {
    const IGeometry::SharedVertexList& vertexList = getVerticesRef();
    auto& vertexListStore = vertexList.template getIDataStoreRefAs<const DataStore<float32>>();

    std::set<float32> pointSet;
    for(usize tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
    {
      pointSet.insert(vertexListStore.getComponentValue(tuple, dimensionIndex));
    }

    return {(pointSet.size() == 1)};
  } catch(const std::bad_cast& exception)
  {
    return MakeErrorResult<bool>(-3000, fmt::format("Could not determine whether the geometry is a plane because an exception was thrown: {}", exception.what()));
  }
}

Result<bool> INodeGeometry0D::isYZPlane() const
{
  return isPlane(0);
}

Result<bool> INodeGeometry0D::isXYPlane() const
{
  return isPlane(2);
}

Result<bool> INodeGeometry0D::isXZPlane() const
{
  return isPlane(1);
}

void INodeGeometry0D::setVertexCoordinate(usize vertId, const Point3D<float32>& coordinate)
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    vertices[offset + i] = coordinate[i];
  }
}

Point3D<float32> INodeGeometry0D::getVertexCoordinate(usize vertId) const
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  Point3D<float32> coordinate;
  for(usize i = 0; i < 3; i++)
  {
    coordinate[i] = vertices.at(offset + i);
  }
  return coordinate;
}

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexAttributeMatrixId() const
{
  return m_VertexAttributeMatrixId;
}

void INodeGeometry0D::setVertexDataId(const OptionalId& vertexDataId)
{
  m_VertexAttributeMatrixId = vertexDataId;
}

AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix()
{
  if(!m_VertexAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix() const
{
  if(!m_VertexAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef()
{
  if(!m_VertexAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry0D::{} threw runtime exception. The geometry with name '{}' does not have a vertex attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef() const
{
  if(!m_VertexAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry0D::{} threw runtime exception. The geometry with name '{}' does not have a vertex attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexAttributeMatrixId.value());
}

DataPath INodeGeometry0D::getVertexAttributeMatrixDataPath() const
{
  return getVertexAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry0D::setVertexAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_VertexAttributeMatrixId = attributeMatrix.getId();
}

void INodeGeometry0D::checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap)
{
  IGeometry::checkUpdatedIdsImpl(updatedIdsMap);

  std::vector<bool> visited(2, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_VertexDataArrayId = nx::core::VisitDataStructureId(m_VertexDataArrayId, updatedId, visited, 0);
    m_VertexAttributeMatrixId = nx::core::VisitDataStructureId(m_VertexAttributeMatrixId, updatedId, visited, 1);
  }
}

Result<> INodeGeometry0D::validate() const
{
  Result<> result;
  usize numVerts = getNumberOfVertices();
  const AttributeMatrix* amPtr = getVertexAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumTuples();
  if(numVerts != amNumTuples)
  {
    return MakeErrorResult(
        -4500, fmt::format("Vertex Geometry '{}' has {} vertices but the vertex Attribute Matrix '{}' has {} total tuples.", getName(), numVerts, getVertexAttributeMatrix()->getName(), amNumTuples));
  }
  return result;
}

} // namespace nx::core

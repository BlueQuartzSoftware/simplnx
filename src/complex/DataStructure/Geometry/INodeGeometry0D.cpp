#include "INodeGeometry0D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
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
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

const INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices() const
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef()
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

const INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

void INodeGeometry0D::setVertices(const INodeGeometry0D::SharedVertexList& vertices)
{
  m_VertexDataArrayId = vertices.getId();
}

void INodeGeometry0D::resizeVertexList(usize size)
{
  getVerticesRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry0D::getNumberOfVertices() const
{
  const auto& vertices = getVerticesRef();
  return vertices.getNumberOfTuples();
}

usize INodeGeometry0D::getNumberOfCells() const
{
  const auto& vertices = getVerticesRef();
  return vertices.getNumberOfTuples();
}

std::optional<INodeGeometry0D::BoundingBox> INodeGeometry0D::getBoundingBox() const
{
  FloatVec3 ll = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  FloatVec3 ur = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

  try
  {
    const IGeometry::SharedVertexList& vertexList = getVerticesRef();
    auto& vertexListStore = vertexList.getIDataStoreRefAs<const DataStore<float32>>();

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
  } catch(const std::bad_cast& exception)
  {
    return {};
  }

  return {std::make_pair(ll, ur)};
}

std::optional<bool> INodeGeometry0D::isPlane(usize dimensionIndex) const
{
  try
  {
    const IGeometry::SharedVertexList& vertexList = getVerticesRef();
    auto& vertexListStore = vertexList.getIDataStoreRefAs<const DataStore<float32>>();

    std::set<float32> pointSet;
    for(usize tuple = 0; tuple < vertexListStore.getNumberOfTuples(); tuple++)
    {
      pointSet.insert(vertexListStore.getComponentValue(tuple, dimensionIndex));
    }

    return (pointSet.size() == 1);
  } catch(const std::bad_cast& exception)
  {
    return {};
  }
}

std::optional<bool> INodeGeometry0D::isYZPlane() const
{
  return isPlane(0);
}

std::optional<bool> INodeGeometry0D::isXYPlane() const
{
  return isPlane(2);
}

std::optional<bool> INodeGeometry0D::isXZPlane() const
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

AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef() const
{
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

H5::ErrorType INodeGeometry0D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = IGeometry::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_VertexDataArrayId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_VertexAttributeMatrixId = ReadH5DataId(groupReader, H5Constants::k_VertexDataTag);

  return error;
}

H5::ErrorType INodeGeometry0D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = IGeometry::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());

  error = WriteH5DataId(groupWriter, m_VertexDataArrayId, H5Constants::k_VertexListTag);
  if(error < 0)
  {
    return error;
  }

  if(m_VertexDataArrayId.has_value())
  {
    usize numVerts = getNumberOfVertices();
    auto datasetWriter = groupWriter.createDatasetWriter("_VertexIndices");
    std::vector<int64> indices(numVerts);
    for(usize i = 0; i < numVerts; i++)
    {
      indices[i] = i;
    }
    error = datasetWriter.writeSpan(H5::DatasetWriter::DimsType{numVerts, 1}, nonstd::span<const int64>{indices});
    if(error < 0)
    {
      return 0;
    }
  }

  error = WriteH5DataId(groupWriter, m_VertexAttributeMatrixId, H5Constants::k_VertexDataTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}

void INodeGeometry0D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  IGeometry::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexDataArrayId == updatedId.first)
    {
      m_VertexDataArrayId = updatedId.second;
    }
    if(m_VertexAttributeMatrixId == updatedId.first)
    {
      m_VertexAttributeMatrixId = updatedId.second;
    }
  }
}
} // namespace complex

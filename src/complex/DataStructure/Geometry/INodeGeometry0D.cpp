#include "INodeGeometry0D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
{
INodeGeometry0D::INodeGeometry0D(DataStructure& ds, std::string name)
: IGeometry(ds, std::move(name))
{
}

INodeGeometry0D::INodeGeometry0D(DataStructure& ds, std::string name, IdType importId)
: IGeometry(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexListId() const
{
  return m_VertexListId;
}

INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices()
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
}

const INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices() const
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
}

INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef()
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
}

const INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
}

void INodeGeometry0D::setVertices(const INodeGeometry0D::SharedVertexList& vertices)
{
  m_VertexListId = vertices.getId();
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

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexDataId() const
{
  return m_VertexDataId;
}

AttributeMatrix* INodeGeometry0D::getVertexData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
}

const AttributeMatrix* INodeGeometry0D::getVertexData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
}

AttributeMatrix& INodeGeometry0D::getVertexDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
}

const AttributeMatrix& INodeGeometry0D::getVertexDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
}

DataPath INodeGeometry0D::getVertexDataPath() const
{
  return getVertexDataRef().getDataPaths().at(0);
}

void INodeGeometry0D::setVertexData(const AttributeMatrix& attributeMatrix)
{
  m_VertexDataId = attributeMatrix.getId();
}

H5::ErrorType INodeGeometry0D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = IGeometry::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_VertexDataId = ReadH5DataId(groupReader, H5Constants::k_VertexDataTag);

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

  error = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_VertexDataId, H5Constants::k_VertexDataTag);
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
    if(m_VertexListId == updatedId.first)
    {
      m_VertexListId = updatedId.second;
    }
    if(m_VertexDataId == updatedId.first)
    {
      m_VertexDataId = updatedId.second;
    }
  }
}
} // namespace complex

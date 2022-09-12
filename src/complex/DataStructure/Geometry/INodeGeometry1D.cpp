#include "INodeGeometry1D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
{
INodeGeometry1D::INodeGeometry1D(DataStructure& ds, std::string name)
: INodeGeometry0D(ds, std::move(name))
{
}

INodeGeometry1D::INodeGeometry1D(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry0D(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeListId() const
{
  return m_EdgeListId;
}

INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges()
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
}

const INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges() const
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
}

INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef()
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
}

const INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
}

void INodeGeometry1D::setEdges(const SharedEdgeList& edges)
{
  m_EdgeListId = edges.getId();
}

void INodeGeometry1D::resizeEdgeList(usize size)
{
  getEdgesRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry1D::getNumberOfEdges() const
{
  const auto& edges = getEdgesRef();
  return edges.getNumberOfTuples();
}

const INodeGeometry1D::ElementDynamicList* INodeGeometry1D::getElementsContainingVert() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_ElementContainingVertId);
}

void INodeGeometry1D::deleteElementsContainingVert()
{
  getDataStructureRef().removeData(m_ElementContainingVertId);
  m_ElementContainingVertId.reset();
}

const INodeGeometry1D::ElementDynamicList* INodeGeometry1D::getElementNeighbors() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_ElementNeighborsId);
}

void INodeGeometry1D::deleteElementNeighbors()
{
  getDataStructureRef().removeData(m_ElementNeighborsId);
  m_ElementNeighborsId.reset();
}

const Float32Array* INodeGeometry1D::getElementCentroids() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_ElementCentroidsId);
}

void INodeGeometry1D::deleteElementCentroids()
{
  getDataStructureRef().removeData(m_ElementCentroidsId);
  m_ElementCentroidsId.reset();
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeDataId() const
{
  return m_EdgeDataId;
}

AttributeMatrix* INodeGeometry1D::getEdgeData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
}

const AttributeMatrix* INodeGeometry1D::getEdgeData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
}

AttributeMatrix& INodeGeometry1D::getEdgeDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
}

const AttributeMatrix& INodeGeometry1D::getEdgeDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
}

DataPath INodeGeometry1D::getEdgeDataPath() const
{
  return getEdgeDataRef().getDataPaths().at(0);
}

void INodeGeometry1D::setEdgeData(const AttributeMatrix& attributeMatrix)
{
  m_EdgeDataId = attributeMatrix.getId();
}

H5::ErrorType INodeGeometry1D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = INodeGeometry0D::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_EdgeListId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
  m_EdgeDataId = ReadH5DataId(groupReader, H5Constants::k_EdgeDataTag);
  m_ElementContainingVertId = ReadH5DataId(groupReader, H5Constants::k_ElementContainingVertTag);
  m_ElementNeighborsId = ReadH5DataId(groupReader, H5Constants::k_ElementNeighborsTag);
  m_ElementCentroidsId = ReadH5DataId(groupReader, H5Constants::k_ElementCentroidTag);

  return error;
}

H5::ErrorType INodeGeometry1D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = INodeGeometry0D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_EdgeDataId, H5Constants::k_EdgeDataTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_ElementContainingVertId, H5Constants::k_ElementContainingVertTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_ElementNeighborsId, H5Constants::k_ElementNeighborsTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_ElementCentroidsId, H5Constants::k_ElementCentroidTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}

void INodeGeometry1D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry0D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexListId == updatedId.first)
    {
      m_VertexListId = updatedId.second;
    }

    if(m_EdgeDataId == updatedId.first)
    {
      m_EdgeDataId = updatedId.second;
    }

    if(m_EdgeListId == updatedId.first)
    {
      m_EdgeListId = updatedId.second;
    }

    if(m_ElementContainingVertId == updatedId.first)
    {
      m_ElementContainingVertId = updatedId.second;
    }

    if(m_ElementNeighborsId == updatedId.first)
    {
      m_ElementNeighborsId = updatedId.second;
    }

    if(m_ElementCentroidsId == updatedId.first)
    {
      m_ElementCentroidsId = updatedId.second;
    }

    if(m_ElementSizesId == updatedId.first)
    {
      m_ElementSizesId = updatedId.second;
    }
  }
}
} // namespace complex

#include "DataGroupIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
DataGroupIO::DataGroupIO() = default;
DataGroupIO::~DataGroupIO() noexcept = default;

Result<> DataGroupIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_EdgeListId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
  m_EdgesContainingVertId = ReadH5DataId(groupReader, H5Constants::k_EdgesContainingVertTag);
  m_EdgeNeighborsId = ReadH5DataId(groupReader, H5Constants::k_EdgeNeighborsTag);
  m_EdgeCentroidsId = ReadH5DataId(groupReader, H5Constants::k_EdgeCentroidTag);
  m_EdgeSizesId = ReadH5DataId(groupReader, H5Constants::k_EdgeSizesTag);

  return BaseGroup::readHdf5(dataStructureReader, groupReader, useEmptyDataStore);
}
Result<> DataGroupIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto err = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(err < 0)
  {
    return err;
  }

  auto errorCode = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgesContainingVertId, H5Constants::k_EdgesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeNeighborsId, H5Constants::k_EdgeNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeCentroidsId, H5Constants::k_EdgeCentroidTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeSizesId, H5Constants::k_EdgeSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5

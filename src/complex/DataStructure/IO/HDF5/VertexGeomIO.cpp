#include "VertexGeomIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
VertexGeomIO::VertexGeomIO() = default;
VertexGeomIO::~VertexGeomIO() noexcept = default;

Result<> VertexGeomIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_VertexSizesId = ReadH5DataId(groupReader, H5Constants::k_VertexSizesTag);

  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}
Result<> VertexGeomIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  herr_t errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_VertexSizesId, H5Constants::k_VertexSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5

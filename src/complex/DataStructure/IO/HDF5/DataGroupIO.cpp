#include "DataGroupIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
DataGroupIO::DataGroupIO() = default;
DataGroupIO::~DataGroupIO() noexcept = default;

Result<> DataGroupIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  return BaseGroup::readHdf5(dataStructureReader, groupReader, useEmptyDataStore);
}
Result<> DataGroupIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto error = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  return m_DataMap.writeH5Group(dataStructureWriter, groupWriter);
}
} // namespace complex::HDF5

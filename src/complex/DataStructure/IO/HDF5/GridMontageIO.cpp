#include "GridMontageIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex::HDF5
{
GridMontageIO::GridMontageIO() = default;
GridMontageIO::~GridMontageIO() noexcept = default;

Result<> GridMontageIO::readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                                 const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
}
Result<> GridMontageIO::writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const
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

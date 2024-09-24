#include "BaseGroupIO.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"

namespace nx::core::HDF5
{
BaseGroupIO::BaseGroupIO() = default;
BaseGroupIO::~BaseGroupIO() noexcept = default;

Result<> BaseGroupIO::ReadBaseGroupData(DataStructureReader& dataStructureReader, BaseGroup& baseGroup, const group_reader_type& parentGroupReader, const std::string& objectName,
                                        DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore)
{
  auto groupReaderResult = parentGroupReader.openGroup(objectName);
  if(groupReaderResult.invalid())
  {
    return ConvertResult(std::move(groupReaderResult));
  }
  auto groupReader = std::move(groupReaderResult.value());

  return ReadDataMap(dataStructureReader, baseGroup.getDataMap(), groupReader, baseGroup.getId(), useEmptyDataStore);
}

Result<> BaseGroupIO::WriteBaseGroupData(DataStructureWriter& dataStructureWriter, const BaseGroup& baseGroup, group_writer_type& parentGroupWriter, bool importable)
{
  auto groupWriterResult = parentGroupWriter.createGroup(baseGroup.getName());
  if(groupWriterResult.invalid())
  {
    return ConvertResult(std::move(groupWriterResult));
  }
  auto groupWriter = std::move(groupWriterResult.value());

  Result<> result = WriteObjectAttributes(dataStructureWriter, baseGroup, groupWriter, importable);
  if(result.invalid())
  {
    return result;
  }

  return WriteDataMap(dataStructureWriter, baseGroup.getDataMap(), groupWriter, importable);
}

Result<> BaseGroupIO::WriteDataMap(DataStructureWriter& dataStructureWriter, const DataMap& dataMap, group_writer_type& parentGroupWriter, bool importable)
{
  for(const auto& [id, dataObject] : dataMap)
  {
    Result<> result = dataStructureWriter.writeDataObject(dataObject.get(), parentGroupWriter);
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}
} // namespace nx::core::HDF5

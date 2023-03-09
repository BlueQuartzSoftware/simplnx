#include "DataGroupIO.hpp"

#include "DataStructureReader.hpp"
#include "complex/DataStructure/DataGroup.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"

namespace complex::HDF5
{
DataGroupIO::DataGroupIO() = default;
DataGroupIO::~DataGroupIO() noexcept = default;

DataObject::Type DataGroupIO::getDataType() const
{
  return DataObject::Type::DataGroup;
}

std::string DataGroupIO::getTypeName() const
{
  return data_type::k_TypeName;
}

Result<> DataGroupIO::readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* dataGroup = DataGroup::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return BaseGroupIO::ReadBaseGroupData(structureReader, *dataGroup, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> DataGroupIO::writeData(DataStructureWriter& dataStructureWriter, const DataGroup& dataGroup, group_writer_type& parentGroup, bool importable) const
{
  auto groupWriter = parentGroup.createGroupWriter(dataGroup.getName());
  return WriteBaseGroupData(dataStructureWriter, dataGroup, parentGroup, importable);
}

Result<> DataGroupIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const
{
  auto* targetData = dynamic_cast<const data_type*>(dataObject);
  if(targetData == nullptr)
  {
    std::string ss = "Provided DataObject could not be cast to the target type";
    return MakeErrorResult(-800, ss);
  }

  return writeData(dataStructureWriter, *targetData, parentWriter, true);
}
} // namespace complex::HDF5

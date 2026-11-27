#include "DataGroupIO.hpp"

#include "DataStructureReader.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::hdf5
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

Result<> DataGroupIO::readData(DataStructureReader& structureReader, const GroupReaderType& parentGroup, const std::string& objectName, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore) const
{
  auto* dataGroupPtr = DataGroup::Import(structureReader.getDataStructure(), objectName, importId, parentId);
  return BaseGroupIO::ReadBaseGroupData(structureReader, *dataGroupPtr, parentGroup, objectName, importId, parentId, useEmptyDataStore);
}

Result<> DataGroupIO::writeData(DataStructureWriter& dataStructureWriter, const DataGroup& dataGroup, GroupWriterType& parentGroup, bool importable)
{
  auto groupWriter = parentGroup.createGroup(dataGroup.getName());
  return WriteBaseGroupData(dataStructureWriter, dataGroup, parentGroup, importable);
}

Result<> DataGroupIO::writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, GroupWriterType& parentWriter) const
{
  return WriteDataObjectImpl(this, dataStructureWriter, dataObject, parentWriter);
}
} // namespace nx::core::hdf5

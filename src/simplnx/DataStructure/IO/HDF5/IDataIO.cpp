#include "IDataIO.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

namespace nx::core::hdf5
{
IDataIO::IDataIO() = default;
IDataIO::~IDataIO() noexcept = default;

DataObject::OptionalIdType IDataIO::ReadDataId(const ObjectReaderType& groupReader, const std::string& tag)
{
  if(!groupReader.isValid())
  {
    return {};
  }

  auto result = groupReader.readScalarAttribute<DataObject::IdType>(tag);
  if(result.invalid())
  {
    return {};
  }
  DataObject::IdType id = result.value();

  return id;
}

Result<> IDataIO::WriteDataId(ObjectWriterType& objectWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag)
{
  if(!objectId.has_value())
  {
    return {};
  }

  DataObject::IdType id = objectId.value();
  return objectWriter.writeScalarAttribute(tag, id);
}

Result<> IDataIO::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, ObjectWriterType& objectWriter, bool importable)
{
  std::string dataTypeName = dataObject.getTypeName();
  objectWriter.writeStringAttribute(constants::k_ObjectTypeTag, dataTypeName);
  objectWriter.writeScalarAttribute(constants::k_ObjectIdTag, dataObject.getId());

  int32 value = (importable ? 1 : 0);
  objectWriter.writeScalarAttribute(constants::k_ImportableTag, value);

  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject.getId());

  return {};
}

Result<> IDataIO::finishImportingData(DataStructure& /*dataStructure*/, const DataPath& /*dataPath*/, const GroupReaderType& /*dataStructureGroup*/) const
{
  return {};
}
} // namespace nx::core::hdf5

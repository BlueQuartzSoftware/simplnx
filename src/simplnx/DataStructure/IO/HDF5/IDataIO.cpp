#include "IDataIO.hpp"

#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include "fmt/format.h"

namespace nx::core::HDF5
{
IDataIO::IDataIO() = default;
IDataIO::~IDataIO() noexcept = default;

DataObject::OptionalId IDataIO::ReadDataId(const object_reader_type& groupReader, const std::string& tag)
{
  if(!groupReader.isValid())
  {
    return {};
  }
  
  auto result = groupReader.readScalarAttribute<DataObject::IdType>(tag);
  if (result.invalid())
  {
    return {};
  }
  DataObject::IdType id = std::move(result.value());

  return id;
}

Result<> IDataIO::WriteDataId(object_writer_type& objectWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag)
{
  if(!objectId.has_value())
  {
    return {};
  }

  DataObject::IdType id = objectId.value();
  return objectWriter.writeScalarAttribute(tag, id);
}

Result<> IDataIO::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, object_writer_type& objectWriter, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject.getId());

  std::string dataTypeName = dataObject.getTypeName();
  objectWriter.writeStringAttribute(Constants::k_ObjectTypeTag, dataTypeName);
  objectWriter.writeScalarAttribute(Constants::k_ObjectIdTag, dataObject.getId());

  int32 value = (importable ? 1 : 0);
  objectWriter.writeScalarAttribute(Constants::k_ImportableTag, value);

  return {};
}
} // namespace nx::core::HDF5

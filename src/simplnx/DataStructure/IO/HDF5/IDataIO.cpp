#include "IDataIO.hpp"

#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

#include "fmt/format.h"

namespace nx::core::HDF5
{
IDataIO::IDataIO() = default;
IDataIO::~IDataIO() noexcept = default;

DataObject::OptionalId IDataIO::ReadDataId(const object_reader_type& groupReader, const std::string& tag)
{
  auto idAttribute = groupReader.getAttribute(tag);
  if(!idAttribute.isValid())
  {
    return {};
  }
  DataObject::IdType id = idAttribute.readAsValue<DataObject::IdType>();
  return id;
}

Result<> IDataIO::WriteDataId(object_writer_type& objectWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag)
{
  if(!objectId.has_value())
  {
    return {};
  }

  auto attribute = objectWriter.createAttribute(tag);
  if(!attribute.isValid())
  {
    std::string ss = fmt::format("Failed to create or open attribute '{}'", tag);
    return MakeErrorResult(-404, ss);
  }
  DataObject::IdType id = objectId.value();
  auto result = attribute.writeValue<DataObject::IdType>(id);
  if(result.invalid())
  {
    std::string ss = fmt::format("Failed to write DataObject ID, '{}' for tag '{}'", id, tag);
    return MakeErrorResult(405, ss);
  }
  return {};
}

Result<> IDataIO::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, object_writer_type& objectWriter, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject.getId());

  auto typeAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectTypeTag);
  auto result = typeAttributeWriter.writeString(dataObject.getTypeName());
  if(result.invalid())
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ObjectTypeTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  auto idAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectIdTag);
  result = idAttributeWriter.writeValue(dataObject.getId());
  if(result.invalid())
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ObjectIdTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  auto importableAttributeWriter = objectWriter.createAttribute(Constants::k_ImportableTag);
  result = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);
  if(result.invalid())
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ImportableTag);
    return MakeErrorResult(result.errors()[0].code, ss);
  }

  return {};
}
} // namespace nx::core::HDF5

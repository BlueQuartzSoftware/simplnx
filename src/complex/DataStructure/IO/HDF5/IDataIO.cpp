#include "IDataIO.hpp"

#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"

#include "complex/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

#include "fmt/format.h"

namespace complex::HDF5
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
  auto error = attribute.writeValue<DataObject::IdType>(id);
  if(error < 0)
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
  auto error = typeAttributeWriter.writeString(dataObject.getTypeName());
  if(error < 0)
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ObjectTypeTag);
    return MakeErrorResult(error, ss);
  }

  auto idAttributeWriter = objectWriter.createAttribute(Constants::k_ObjectIdTag);
  error = idAttributeWriter.writeValue(dataObject.getId());
  if(error < 0)
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ObjectIdTag);
    return MakeErrorResult(error, ss);
  }

  auto importableAttributeWriter = objectWriter.createAttribute(Constants::k_ImportableTag);
  error = importableAttributeWriter.writeValue<int32>(importable ? 1 : 0);
  if(error < 0)
  {
    std::string ss = fmt::format("Error writing DataObject attribute: {}", Constants::k_ImportableTag);
    return MakeErrorResult(error, ss);
  }

  return {};
}
} // namespace complex::HDF5

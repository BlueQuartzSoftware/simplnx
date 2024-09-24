#include "IDataIO.hpp"

#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"

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
  DataObject::IdType id;
  auto objectType = groupReader.getObjectType();
  try
  {
    if(objectType == HighFive::ObjectType::Group)
    {
      dynamic_cast<const GroupIO&>(groupReader).readAttribute(tag, id);
    }
    else if(objectType == HighFive::ObjectType::Dataset)
    {
      dynamic_cast<const DatasetIO&>(groupReader).readAttribute(tag, id);
    }
  }
  catch (const std::exception& e)
  {
    return {};
  }
  
  return id;
}

Result<> IDataIO::WriteDataId(object_writer_type& objectWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag)
{
  if(!objectId.has_value())
  {
    return {};
  }

  DataObject::IdType id = objectId.value();
  auto objectType = objectWriter.getObjectType();
  if(objectType == HighFive::ObjectType::Group)
  {
    dynamic_cast<GroupIO&>(objectWriter).createAttribute(tag, id);
  }
  if(objectType == HighFive::ObjectType::Dataset)
  {
    dynamic_cast<DatasetIO&>(objectWriter).createAttribute(tag, id);
  }
  
  return {};
}

Result<> IDataIO::WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, object_writer_type& objectWriter, bool importable)
{
  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject.getId());

  std::string dataTypeName = dataObject.getTypeName();
  auto objectType = objectWriter.getObjectType();
  if(objectType == HighFive::ObjectType::Group)
  {
    auto& groupWriter = dynamic_cast<GroupIO&>(objectWriter);
    groupWriter.createAttribute(Constants::k_ObjectTypeTag, dataTypeName);
    groupWriter.createAttribute(Constants::k_ObjectIdTag, dataObject.getId());

    int32 value = (importable ? 1 : 0);
    groupWriter.createAttribute(Constants::k_ImportableTag, value);
  }
  else if(objectType == HighFive::ObjectType::Dataset)
  {
    auto& datasetWriter = dynamic_cast<DatasetIO&>(objectWriter);
    datasetWriter.createAttribute(Constants::k_ObjectTypeTag, dataTypeName);
    datasetWriter.createAttribute(Constants::k_ObjectIdTag, dataObject.getId());

    int32 value = (importable ? 1 : 0);
    datasetWriter.createAttribute(Constants::k_ImportableTag, value);
  }  
  
  return {};
}
} // namespace nx::core::HDF5

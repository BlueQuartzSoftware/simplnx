#include "IDataIO.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <fmt/format.h>

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
  if(result.invalid())
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
  std::string dataTypeName = dataObject.getTypeName();
  objectWriter.writeStringAttribute(Constants::k_ObjectTypeTag, dataTypeName);
  objectWriter.writeScalarAttribute(Constants::k_ObjectIdTag, dataObject.getId());

  int32 value = (importable ? 1 : 0);
  objectWriter.writeScalarAttribute(Constants::k_ImportableTag, value);

  // Metadata
  if(!dataObject.getMetadata().isEmpty())
  {
    Result<> metaDataResult = objectWriter.writeStringAttribute(Constants::k_ObjectMetaTag, dataObject.getMetadata().toJson().dump());
    if(metaDataResult.invalid())
    {
      return metaDataResult;
    }
  }

  // Add to DataStructureWriter for use in linking
  dataStructureWriter.addWriter(objectWriter, dataObject.getId());

  return {};
}

Result<> IDataIO::ReadMetaData(DataStructureReader& dataStructureReader, const DataObject::IdType& objectId, const object_reader_type& objectReader)
{
  if(!objectReader.hasAttribute(Constants::k_ObjectMetaTag))
  {
    return {};
  }

  DataStructure& dataStructure = dataStructureReader.getDataStructure();
  if(!dataStructure.containsData(objectId))
  {
    auto errorStr = fmt::format("DataStructure does not contain DataObject with ID '{}'", objectId);
    return MakeErrorResult(-9700, errorStr);
  }

  auto& dataObject = dataStructure.getDataRef(objectId);

  Result<std::string> jsonResult = objectReader.readStringAttribute(Constants::k_ObjectMetaTag);
  if(jsonResult.invalid())
  {
    return ConvertResult(std::move(jsonResult));
  }

  dataObject.getMetadata().fromJson(jsonResult.value());

  return {};
}

Result<> IDataIO::ReadMetaData(DataStructureReader& dataStructureReader, const group_reader_type& parentReader, const std::string& objectName, const DataObject::IdType& objectId)
{
  if(parentReader.isGroup(objectName))
  {
    const auto& group = parentReader.openGroup(objectName);
    return ReadMetaData(dataStructureReader, objectId, group);
  }
  else
  {
    const auto& dataset = parentReader.openDataset(objectName);
    return ReadMetaData(dataStructureReader, objectId, dataset);
  }
}

Result<> IDataIO::finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const
{
  return {};
}
} // namespace nx::core::HDF5

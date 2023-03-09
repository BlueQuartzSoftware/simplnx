#include "DataStructureWriter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

#include "complex/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include "fmt/format.h"

namespace complex::HDF5
{
DataStructureWriter::DataStructureWriter()
{
  auto* instance = Application::GetOrCreateInstance();
  m_IOManager = std::dynamic_pointer_cast<DataIOManager>(instance->getIOManager("HDF5"));
}

DataStructureWriter::~DataStructureWriter() noexcept = default;

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath)
{
  auto fileWriterResult = complex::HDF5::FileWriter::CreateFile(filepath);
  if(fileWriterResult.invalid())
  {
    auto error = fileWriterResult.errors()[0];
    return MakeErrorResult(error.code, error.message);
  }
  complex::HDF5::FileWriter fileWriter = std::move(fileWriterResult.value());
  return WriteFile(dataStructure, fileWriter);
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, complex::HDF5::FileWriter& fileWriter)
{
  HDF5::DataStructureWriter dataStructureWriter;
  auto groupWriter = fileWriter.createGroupWriter(Constants::k_DataStructureTag);
  return dataStructureWriter.writeDataStructure(dataStructure, groupWriter);
}

Result<> DataStructureWriter::writeDataObject(const DataObject* dataObject, complex::HDF5::GroupWriter& parentGroup)
{
  // Check if data has already been written
  if(hasDataBeenWritten(dataObject))
  {
    // Create an HDF5 link
    return writeDataObjectLink(dataObject, parentGroup);
  }
  else
  {
    // Write new data
    auto factory = m_IOManager->getFactoryAs<IDataIO>(dataObject->getTypeName());
    if(factory == nullptr)
    {
      std::string ss = fmt::format("Could not find IO factory for datatype: {}", dataObject->getTypeName());
      return MakeErrorResult(-5, ss);
    }

    auto result = factory->writeDataObject(*this, dataObject, parentGroup);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> DataStructureWriter::writeDataMap(const DataMap& dataMap, complex::HDF5::GroupWriter& parentGroup)
{
  for(auto [key, object] : dataMap)
  {
    Result<> result = writeDataObject(object.get(), parentGroup);
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> DataStructureWriter::writeDataStructure(const DataStructure& dataStructure, complex::HDF5::GroupWriter& groupWriter)
{
  auto idAttribute = groupWriter.createAttribute(Constants::k_NextIdTag);
  complex::HDF5::ErrorType err = idAttribute.writeValue(dataStructure.getNextId());
  if(err < 0)
  {
    std::string ss = "Failed to write DataStructure to HDF5 group";
    return MakeErrorResult(err, ss);
  }

  return writeDataMap(dataStructure.getDataMap(), groupWriter);
}

Result<> DataStructureWriter::writeDataObjectLink(const DataObject* dataObject, complex::HDF5::GroupWriter& parentGroup)
{
  auto objectPath = getPathForObjectId(dataObject->getId());
  auto error = parentGroup.createLink(objectPath);
  if(error < 0)
  {
    const std::string ss = fmt::format("Failed to create data link at path {}", objectPath);
    return MakeErrorResult(error, ss);
  }

  // NeighborList extra data link
  if(const auto* neighborList = dynamic_cast<const INeighborList*>(dataObject))
  {
    auto numNeighborsName = neighborList->getNumNeighborsArrayName();
    auto dataPath = getPathForObjectSibling(dataObject->getId(), numNeighborsName);
    error = parentGroup.createLink(dataPath);
    if(error < 0)
    {
      const std::string ss = fmt::format("Failed to create data link at path {}", dataPath);
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}

bool DataStructureWriter::hasDataBeenWritten(const DataObject* targetObject) const
{
  if(targetObject == nullptr)
  {
    return false;
  }
  return hasDataBeenWritten(targetObject->getId());
}

bool DataStructureWriter::hasDataBeenWritten(DataObject::IdType targetId) const
{
  return m_IdMap.find(targetId) != m_IdMap.end();
}

std::string DataStructureWriter::getPathForObjectId(DataObject::IdType objectId) const
{
  if(!hasDataBeenWritten(objectId))
  {
    return "";
  }
  return m_IdMap.at(objectId);
}

std::string DataStructureWriter::getParentPathForObjectId(DataObject::IdType objectId) const
{
  auto objectPath = getPathForObjectId(objectId);
  if(objectPath.empty())
  {
    return objectPath;
  }
  auto lastIndex = objectPath.find_last_of('/');
  if(lastIndex < 0)
  {
    return objectPath;
  }
  return objectPath.substr(0, lastIndex);
}

std::string DataStructureWriter::getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const
{
  auto objectPath = getParentPathForObjectId(objectId);
  if(!objectPath.empty())
  {
    objectPath += "/";
  }
  objectPath += siblingName;
  return objectPath;
}

void DataStructureWriter::clearIdMap()
{
  m_IdMap.clear();
  m_ParentId = 0;
}

void DataStructureWriter::addWriter(complex::HDF5::ObjectWriter& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.getObjectPath();
}
} // namespace complex::HDF5

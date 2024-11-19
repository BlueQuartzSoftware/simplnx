#include "DataStructureWriter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/IDataIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "fmt/format.h"

namespace nx::core::HDF5
{
DataStructureWriter::DataStructureWriter()
{
  auto instance = Application::GetOrCreateInstance();
  m_IOManager = std::dynamic_pointer_cast<DataIOManager>(instance->getIOManager("HDF5"));
}

DataStructureWriter::~DataStructureWriter() noexcept = default;

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath)
{
  auto fileWriter = nx::core::HDF5::FileIO::WriteFile(filepath);
  if(fileWriter.isValid() == false)
  {
    return MakeErrorResult(-8054, fmt::format("Failed to create file at path {}", filepath.string()));
  }
  return WriteFile(dataStructure, fileWriter);
}

Result<> DataStructureWriter::WriteFile(const DataStructure& dataStructure, nx::core::HDF5::FileIO& FileIO)
{
  HDF5::DataStructureWriter dataStructureWriter;
  auto groupIOResult = FileIO.createGroup(Constants::k_DataStructureTag);
  if(groupIOResult.invalid())
  {
    return ConvertResult(std::move(groupIOResult));
  }
  auto groupIO = std::move(groupIOResult.value());
  return dataStructureWriter.writeDataStructure(dataStructure, groupIO);
}

Result<> DataStructureWriter::writeDataObject(const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup)
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

Result<> DataStructureWriter::writeDataMap(const DataMap& dataMap, nx::core::HDF5::GroupIO& parentGroup)
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

Result<> DataStructureWriter::writeDataStructure(const DataStructure& dataStructure, nx::core::HDF5::GroupIO& groupIO)
{
  if(!groupIO.isValid())
  {
    std::string ss = "Failed to write DataStructure to HDF5 group";
    return MakeErrorResult(-700, ss);
  }
  groupIO.writeScalarAttribute(Constants::k_NextIdTag, dataStructure.getNextId());
  return writeDataMap(dataStructure.getDataMap(), groupIO);
}

Result<> DataStructureWriter::writeDataObjectLink(const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup)
{
  auto objectPath = getPathForObjectId(dataObject->getId());
  auto result = parentGroup.createLink(objectPath);
  if(result.invalid())
  {
    return result;
  }

  // NeighborList extra data link
  if(const auto* neighborList = dynamic_cast<const INeighborList*>(dataObject))
  {
    auto numNeighborsName = neighborList->getNumNeighborsArrayName();
    auto dataPath = getPathForObjectSibling(dataObject->getId(), numNeighborsName);
    result = parentGroup.createLink(dataPath);
    if(result.invalid())
    {
      return result;
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
}

void DataStructureWriter::addWriter(nx::core::HDF5::ObjectIO& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.getObjectPath();
}
} // namespace nx::core::HDF5

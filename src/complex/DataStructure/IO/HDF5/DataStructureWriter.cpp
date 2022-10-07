#include "DataStructureWriter.hpp"

#include "DataIOManager.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

#include "fmt/format.h"

namespace complex::HDF5
{
DataStructureWriter::DataStructureWriter()
{
  auto* instance = Application::GetOrCreateInstance();
  m_IOManager = std::dynamic_pointer_cast<DataIOManager>(instance->getIOManager("HDF5"));
}

DataStructureWriter::~DataStructureWriter() noexcept = default;

Result<> DataStructureWriter::writeDataObject(const DataObject* dataObject, H5::GroupWriter& parentGroup)
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

Result<> DataStructureWriter::writeDataMap(const DataMap& dataMap, H5::GroupWriter& parentGroup)
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

Result<> DataStructureWriter::writeDataObjectLink(const DataObject* dataObject, H5::GroupWriter& parentGroup)
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

void DataStructureWriter::addWriter(H5::ObjectWriter& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.getObjectPath();
}
} // namespace complex::HDF5

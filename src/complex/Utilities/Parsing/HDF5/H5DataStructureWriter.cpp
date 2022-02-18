#include "H5DataStructureWriter.hpp"

#include "complex/DataStructure/INeighborList.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

using namespace complex;

H5::DataStructureWriter::DataStructureWriter()
{
}

H5::DataStructureWriter::~DataStructureWriter() = default;

H5::ErrorType H5::DataStructureWriter::writeDataObject(const DataObject* dataObject, GroupWriter& parentGroup)
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
    auto err = dataObject->writeHdf5(*this, parentGroup);
    return err;
  }
}

H5::ErrorType H5::DataStructureWriter::writeDataObjectLink(const DataObject* dataObject, H5::GroupWriter& parentGroup)
{
  auto objectPath = getPathForObjectId(dataObject->getId());
  auto err = parentGroup.createLink(objectPath);

  if(err < 0)
  {
    return err;
  }

  // NeighborList extra data link
  if(const auto* neighborList = dynamic_cast<const INeighborList*>(dataObject))
  {
    auto numNeighborsName = neighborList->getNumNeighborsArrayName();
    auto dataPath = getPathForObjectSibling(dataObject->getId(), numNeighborsName);
    err = parentGroup.createLink(dataPath);
  }
  return err;
}

bool H5::DataStructureWriter::hasDataBeenWritten(const DataObject* targetObject) const
{
  if(targetObject == nullptr)
  {
    return false;
  }
  return hasDataBeenWritten(targetObject->getId());
}

bool H5::DataStructureWriter::hasDataBeenWritten(DataObject::IdType targetId) const
{
  return m_IdMap.find(targetId) != m_IdMap.end();
}

std::string H5::DataStructureWriter::getPathForObjectId(DataObject::IdType objectId) const
{
  if(!hasDataBeenWritten(objectId))
  {
    return "";
  }
  return m_IdMap.at(objectId);
}

std::string H5::DataStructureWriter::getParentPathForObjectId(DataObject::IdType objectId) const
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

std::string H5::DataStructureWriter::getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const
{
  auto objectPath = getParentPathForObjectId(objectId);
  if(!objectPath.empty())
  {
    objectPath += "/";
  }
  objectPath += siblingName;
  return objectPath;
}

void H5::DataStructureWriter::clearIdMap()
{
  m_IdMap.clear();
  m_ParentId = 0;
}

void H5::DataStructureWriter::addH5Writer(H5::ObjectWriter& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.getObjectPath();
}

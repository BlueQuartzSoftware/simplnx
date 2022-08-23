#include "ZarrStructureWriter.hpp"

#include "complex/DataStructure/INeighborList.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;

Zarr::DataStructureWriter::DataStructureWriter()
{
}

Zarr::DataStructureWriter::~DataStructureWriter() = default;

Zarr::ErrorType Zarr::DataStructureWriter::writeDataObject(const DataObject* dataObject, FileVec::IGroup& parentGroup)
{
  return dataObject->writeZarr(*this, parentGroup);
}

bool Zarr::DataStructureWriter::hasDataBeenWritten(const DataObject* targetObject) const
{
  if(targetObject == nullptr)
  {
    return false;
  }
  return hasDataBeenWritten(targetObject->getId());
}

bool Zarr::DataStructureWriter::hasDataBeenWritten(DataObject::IdType targetId) const
{
  return m_IdMap.find(targetId) != m_IdMap.end();
}

std::filesystem::path Zarr::DataStructureWriter::getPathForObjectId(DataObject::IdType objectId) const
{
  if(!hasDataBeenWritten(objectId))
  {
    return "";
  }
  return m_IdMap.at(objectId);
}

std::filesystem::path Zarr::DataStructureWriter::getParentPathForObjectId(DataObject::IdType objectId) const
{
  return getPathForObjectId(objectId).parent_path();
}

std::filesystem::path Zarr::DataStructureWriter::getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const
{
  return getParentPathForObjectId(objectId) / siblingName;
}

void Zarr::DataStructureWriter::clearIdMap()
{
  m_IdMap.clear();
}

void Zarr::DataStructureWriter::addZarrWriter(FileVec::BaseCollection& objectWriter, DataObject::IdType objectId)
{
  m_IdMap[objectId] = objectWriter.path();
}

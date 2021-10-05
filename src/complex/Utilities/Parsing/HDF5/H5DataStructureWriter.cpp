#include "H5DataStructureWriter.hpp"

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
  auto objWriter = getH5WriterForObjectId(dataObject->getId());
  auto err = parentGroup.createLink(objWriter.get());
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

std::shared_ptr<H5::ObjectWriter> H5::DataStructureWriter::getH5WriterForObjectId(DataObject::IdType objectId) const
{
  if(!hasDataBeenWritten(objectId))
  {
    return 0;
  }
  return m_IdMap.at(objectId);
}

void H5::DataStructureWriter::clearIdMap()
{
  m_IdMap.clear();
  m_ParentId = 0;
}

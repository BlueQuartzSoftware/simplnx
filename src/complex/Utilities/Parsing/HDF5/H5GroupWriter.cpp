#include "H5GroupWriter.hpp"

#include <H5Gpublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::GroupWriter::GroupWriter()
: ObjectWriter()
{
}

H5::GroupWriter::GroupWriter(H5::IdType parentId, const std::string& groupName)
: ObjectWriter(parentId)
{
  // Check if group exists
  HDF_ERROR_HANDLER_OFF
  auto status = H5Gget_objinfo(parentId, groupName.c_str(), 0, NULL);
  HDF_ERROR_HANDLER_ON

  if(status == 0) // if group exists...
  {
    m_GroupId = H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT);
  }
  else // if group does not exist...
  {
    m_GroupId = H5Gcreate(parentId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}

H5::GroupWriter::~GroupWriter()
{
  closeHdf5();
}

void H5::GroupWriter::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(m_GroupId);
    m_GroupId = 0;
  }
}

bool H5::GroupWriter::isValid() const
{
  return getId() > 0;
}

H5::IdType H5::GroupWriter::getId() const
{
  return m_GroupId;
}

H5::GroupWriter H5::GroupWriter::createGroupWriter(const std::string& childName)
{
  if(!isValid())
  {
    return GroupWriter();
  }

  return GroupWriter(getId(), childName);
}

H5::DatasetWriter H5::GroupWriter::createDatasetWriter(const std::string& childName)
{
  if(!isValid())
  {
    return DatasetWriter();
  }

  return DatasetWriter(getId(), childName);
}

H5::ErrorType H5::GroupWriter::createLink(const H5::ObjectWriter* targetObject)
{
  auto err = H5Lcreate_hard(targetObject->getParentId(), targetObject->getName().c_str(), getId(), targetObject->getName().c_str(), H5P_DEFAULT, H5P_DEFAULT);
  return err;
}

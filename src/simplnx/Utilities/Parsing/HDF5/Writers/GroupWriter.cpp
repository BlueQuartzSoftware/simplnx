#include "GroupWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

#include <H5Gpublic.h>

namespace nx::core::HDF5
{
GroupWriter::GroupWriter()
: ObjectWriter()
{
}

GroupWriter::GroupWriter(IdType parentId, IdType objectId)
: ObjectWriter(parentId, objectId)
{
}

GroupWriter::GroupWriter(IdType parentId, const std::string& groupName)
: ObjectWriter(parentId)
{
  // Check if group exists
  HDF_ERROR_HANDLER_OFF
  auto status = H5Gget_objinfo(parentId, groupName.c_str(), 0, NULL);
  HDF_ERROR_HANDLER_ON

  if(status == 0) // if group exists...
  {
    setId(H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT));
  }
  else // if group does not exist...
  {
    setId(H5Gcreate(parentId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  }
}

GroupWriter::~GroupWriter()
{
  closeHdf5();
}

void GroupWriter::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(getId());
    setId(0);
  }
}

bool GroupWriter::isValid() const
{
  return getId() > 0;
}

GroupWriter GroupWriter::createGroupWriter(const std::string& childName)
{
  if(!isValid())
  {
    return GroupWriter();
  }

  return GroupWriter(getId(), childName);
}

DatasetWriter GroupWriter::createDatasetWriter(const std::string& childName)
{
  if(!isValid())
  {
    return DatasetWriter();
  }

  return DatasetWriter(getId(), childName);
}

Result<> GroupWriter::createLink(const std::string& objectPath)
{
  if(objectPath.empty())
  {
    return MakeErrorResult(-102, "Cannot link to an empty path");
  }
  size_t index = objectPath.find_last_of('/');
  if(index > 0)
  {
    index++;
  }
  std::string objectName = objectPath.substr(index);

  herr_t errorCode = H5Lcreate_hard(getFileId(), objectPath.c_str(), getId(), objectName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  if(errorCode < 0)
  {
    return MakeErrorResult(errorCode, fmt::format("Error Creating Link to path {}", objectPath));
  }
  return {};
}
} // namespace nx::core::HDF5

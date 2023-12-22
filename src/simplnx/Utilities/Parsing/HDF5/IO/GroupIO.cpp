#include "GroupIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "fmt/format.h"

#include <H5Gpublic.h>
#include <H5Opublic.h>

#include <iostream>

namespace nx::core::HDF5
{
IdType getGroupId(IdType parentId, const std::string& groupName)
{
  // Check if group exists
  HDF_ERROR_HANDLER_OFF
  auto status = H5Gget_objinfo(parentId, groupName.c_str(), 0, NULL);
  HDF_ERROR_HANDLER_ON

  if(status == 0) // if group exists...
  {
    return H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT);
  }
  else // if group does not exist...
  {
    return H5Gcreate(parentId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}

GroupIO::GroupIO() = default;

GroupIO::GroupIO(IdType parentId, const std::string& groupName)
: ObjectIO(parentId, getGroupId(parentId, groupName))
{
}

GroupIO::GroupIO(IdType parentId, IdType objectId)
: ObjectIO(parentId, objectId)
{
}

GroupIO::~GroupIO() noexcept
{
  closeHdf5();
}

void GroupIO::closeHdf5()
{
  if(isValid())
  {
    H5Gclose(getId());
    setId(0);
  }
}

GroupIO GroupIO::openGroup(const std::string& name) const
{
  if(!isValid())
  {
    return GroupIO();
  }

  return GroupIO(getId(), name);
}

std::shared_ptr<GroupIO> GroupIO::openGroupPtr(const std::string& name) const
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<GroupIO>(getId(), name);
}

DatasetIO GroupIO::openDataset(const std::string& name) const
{
  if(!isValid())
  {
    return DatasetIO();
  }

  return DatasetIO(getId(), name);
}

std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& name) const
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(getId(), name);
}

ObjectIO GroupIO::openObject(const std::string& name) const
{
  if(!isValid())
  {
    return ObjectIO();
  }

  return ObjectIO(getId(), name);
}

size_t GroupIO::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  SizeType numChildren;
  auto err = H5Gget_num_objs(getId(), &numChildren);
  if(err < 0)
  {
    return 0;
  }
  return numChildren;
}

std::vector<std::string> GroupIO::getChildNames() const
{
  std::vector<std::string> childNames;
  if(!isValid())
  {
    return childNames;
  }

  constexpr size_t size = 1024;
  char buffer[size];

  const size_t numChildren = getNumChildren();
  for(size_t i = 0; i < numChildren; i++)
  {
    auto err = H5Gget_objname_by_idx(getId(), i, buffer, size);
    if(err >= 0)
    {
      childNames.push_back(GetNameFromBuffer(buffer));
    }
  }

  return childNames;
}

bool GroupIO::isGroup(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  bool isGroup = true;
  H5O_info_t objectInfo{};
  auto error = H5Oget_info_by_name(getId(), childName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error in methd H5Gget_objinfo" << std::endl;
    return false;
  }
  switch(objectInfo.type)
  {
  case H5O_TYPE_GROUP:
    isGroup = true;
    break;
  case H5O_TYPE_DATASET:
    isGroup = false;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    isGroup = false;
    break;
  default:
    isGroup = false;
  }
  return isGroup;
}

bool GroupIO::isDataset(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  bool isDataset = true;
  H5O_info_t objectInfo{};
  auto error = H5Oget_info_by_name(getId(), childName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    std::cout << "Error in methd H5Gget_objinfo" << std::endl;
    return false;
  }
  switch(objectInfo.type)
  {
  case H5O_TYPE_GROUP:
    isDataset = false;
    break;
  case H5O_TYPE_DATASET:
    isDataset = true;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    isDataset = false;
    break;
  default:
    isDataset = false;
  }
  return isDataset;
}

GroupIO GroupIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    return GroupIO();
  }

  return GroupIO(getId(), childName);
}

std::shared_ptr<GroupIO> GroupIO::createGroupPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<GroupIO>(getId(), childName);
}

DatasetIO GroupIO::openDataset(const std::string& childName)
{
  if(!isValid())
  {
    return DatasetIO();
  }

  return DatasetIO(getId(), childName);
}

std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(getId(), childName);
}

DatasetIO GroupIO::createDataset(const std::string& childName)
{
  if(!isValid())
  {
    return DatasetIO();
  }

  DatasetIO dataset(getId(), childName);
  return std::move(dataset);
}

std::shared_ptr<DatasetIO> GroupIO::createDatasetPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(getId(), childName);
}

Result<> GroupIO::createLink(const std::string& objectPath)
{
  if(objectPath.empty())
  {
    return MakeErrorResult(-105, "Cannot create link with empty path");
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
    return MakeErrorResult(errorCode, fmt::format("Failed to create link to path '{}'", objectPath));
  }
  return {};
}

// -----------------------------------------------------------------------------
} // namespace nx::core::HDF5

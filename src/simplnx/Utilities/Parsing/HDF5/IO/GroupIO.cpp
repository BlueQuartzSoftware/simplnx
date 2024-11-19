#include "GroupIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
//#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Opublic.h>
#include <H5Gpublic.h>
#include <H5Dpublic.h>

#include "fmt/format.h"

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

#if 0
HighFive::Group GroupIO::Open(const std::filesystem::path& filepath, const std::string& objectPath)
{
  auto file = HighFive::File(filepath.string(), HighFive::File::ReadWrite);
  auto objPathVec = ObjectIO::ObjectPathToVec(objectPath);
  auto group = file.getGroup(objPathVec[0]);
  usize count = objPathVec.size();
  if(count == 1)
  {
    return group;
  }

  for(usize i = 1; i < count; i++)
  {
    group = group.getGroup(objPathVec[i]);
  }
  return group;
}
#endif

GroupIO::GroupIO() = default;

GroupIO::GroupIO(GroupIO& parentGroup, const std::string& groupName, hid_t groupId)
: ObjectIO(&parentGroup, groupName)
{
  setId(groupId);
}

GroupIO::~GroupIO() noexcept
{
  close();
}

void GroupIO::close()
{
  if(getId() > 0)
  {
    H5Gclose(getId());
    setId(0);
  }
}

Result<GroupIO> GroupIO::openGroup(const std::string& name) const
{
  if(!isGroup(name))
  {
    std::string ss = fmt::format("Could not open Group '{}'. Child object does not exist or object is not a Group", name);
    return MakeErrorResult<GroupIO>(-404, ss);
  }
  hid_t groupId = H5Gopen(getId(), name.c_str(), H5P_DEFAULT);
  if (groupId <= 0)
  {
    std::string ss = fmt::format("Failed to open Group '{}'.", name);
    return MakeErrorResult<GroupIO>(-405, ss);
  }
  return {GroupIO(const_cast<GroupIO&>(*this), name, groupId)};
}

#if 0
std::shared_ptr<GroupIO> GroupIO::openGroupPtr(const std::string& name) const
{
  if(!isGroup(name))
  {
    return nullptr;
  }
  hid_t groupId = H5Gopen(getId(), name.c_str(), H5P_DEFAULT);
  return std::make_shared<GroupIO>(const_cast<GroupIO&>(*this), name, groupId);
}
#endif

Result<DatasetIO> GroupIO::openDataset(const std::string& name) const
{
  if(!isDataset(name))
  {
    std::string ss = fmt::format("Could not open Dataset '{}'. Child object does not exist or object is not a Dataset", name);
    return MakeErrorResult<DatasetIO>(-405, ss);
  }
  return {DatasetIO(const_cast<GroupIO&>(*this), name)};
}

#if 0
std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& name) const
{
  if(!isDataset(name))
  {
    return nullptr;
  }
  return std::make_shared<DatasetIO>(const_cast<GroupIO&>(*this), name);
}
#endif

usize GroupIO::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  hsize_t numChildren = 0;
  H5Gget_num_objs(getId(), &numChildren);
  return numChildren;
}

std::string GroupIO::getChildNameByIdx(hsize_t idx) const
{
  const size_t size = 1024;
  char buffer[size];
  H5Gget_objname_by_idx(getId(), idx, buffer, size);
  return GetNameFromBuffer(buffer);
}

std::vector<std::string> GroupIO::getChildNames() const
{
  if(!isValid())
  {
    return {};
  }

  usize numChildren = getNumChildren();
  std::vector<std::string> names(numChildren);
  for(usize i = 0; i < numChildren; i++)
  {
    names[i] = getChildNameByIdx(i);
  }
  return names;
}

bool GroupIO::isGroup(const std::string& childName) const
{
  return getObjectType(childName) == ObjectType::Group;
}

bool GroupIO::isDataset(const std::string& childName) const
{
  return getObjectType(childName) == ObjectType::Dataset;
}

bool GroupIO::exists(const std::string& childName) const
{
  return getObjectType(childName) != ObjectType::Unknown;
}

ObjectIO::ObjectType GroupIO::getObjectType(const std::string& childName) const
{
  if(!isValid())
  {
    return ObjectType::Unknown;
  }

  herr_t error = 1;
  H5O_info_t objectInfo{};

  error = H5Oget_info_by_name(getId(), childName.c_str(), &objectInfo, H5P_DEFAULT);
  if(error < 0)
  {
    return ObjectType::Unknown;
  }

  int32 objectType = objectInfo.type;
  switch (objectType)
  {
  case H5O_TYPE_GROUP:
    return ObjectType::Group;
    break;
  case H5O_TYPE_DATASET:
    return ObjectType::Dataset;
    break;
  case H5O_TYPE_NAMED_DATATYPE:
    break;
  default:
    break;
  }

  return ObjectType::Unknown;
}

Result<GroupIO> GroupIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current group is not valid", childName);
    return MakeErrorResult<GroupIO>(-505, ss);
  }
  hid_t groupId = -1;
  if(isGroup(childName))
  {
    groupId = H5Gopen(getId(), childName.c_str(), H5P_DEFAULT);
  }
  else if(!exists(childName))
  {
    groupId = H5Gcreate(getId(), childName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
  if(groupId > 0)
  {
    return {GroupIO(*this, childName, groupId)};
  }

  std::string ss = fmt::format("Failed to create HDF5 group '{}' at path: ", childName, getObjectPath());
  return MakeErrorResult<GroupIO>(-722, ss);
}

#if 0
std::shared_ptr<GroupIO> GroupIO::createGroupPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }
  if(m_Group.exist(childName) && !isGroup(childName))
  {
    return nullptr;
  }
  auto childGroup = m_Group.createGroup(childName);
  return std::make_shared<GroupIO>(*this, std::move(childGroup), childName);
}
#endif

Result<DatasetIO> GroupIO::openDataset(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot open Dataset '{}'. Current object is not valid.", childName);
    return MakeErrorResult<DatasetIO>(-406, ss);
  }
  hid_t dataId = -1;
  if(isDataset(childName) || !exists(childName))
  {
    return {DatasetIO(*this, childName)};
  }

  std::string ss = fmt::format("Failed to create HDF5 dataset '{}' at path: ", childName, getObjectPath());
  return MakeErrorResult<DatasetIO>(-722, ss);
}

std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }
  if(!isDataset(childName))
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(*this, childName);
}

Result<DatasetIO> GroupIO::createDataset(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Dataset '{}' as the current Group is not valid.", childName);
    return MakeErrorResult<DatasetIO>(-504, ss);
  }

  DatasetIO dataset(getId(), childName);
  return dataset;
}

std::shared_ptr<DatasetIO> GroupIO::createDatasetPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(*this, childName);
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

  herr_t errorCode = H5Lcreate_hard(getParentId(), objectPath.c_str(), getId(), objectName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  if (errorCode < 0)
  {
    return MakeErrorResult(errorCode, fmt::format("Error creating link to path: {}", objectPath));
  }
  return {};
}
// -----------------------------------------------------------------------------
} // namespace nx::core::HDF5

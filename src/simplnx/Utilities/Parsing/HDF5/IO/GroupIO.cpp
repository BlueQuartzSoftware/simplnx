#include "GroupIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <H5Dpublic.h>
#include <H5Gpublic.h>
#include <H5Opublic.h>

#include <fmt/format.h>

#include <iostream>
#include <mutex>

namespace nx::core::HDF5
{
/**
 * @brief Opens an existing group or creates a missing group.
 * @param parentId Identifies the parent object. It must remain valid.
 * @param groupName Identifies the child group.
 * @return Open group identifier, or a negative HDF5 identifier on failure.
 * @pre The caller does not hold Support::ApiLock().
 */
IdType getGroupId(IdType parentId, const std::string& groupName)
{
  // Keep the probe and its dependent open or create operation atomic to HDF5 callers.
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());

  HDF_ERROR_HANDLER_OFF
  auto status = H5Gget_objinfo(parentId, groupName.c_str(), 0, NULL);
  HDF_ERROR_HANDLER_ON

  if(status == 0)
  {
    return H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT);
  }
  else
  {
    return H5Gcreate(parentId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}

GroupIO::GroupIO() = default;

GroupIO::GroupIO(hid_t parentId, const std::string& groupName, hid_t groupId)
: ObjectIO(parentId, groupName)
{
  setId(groupId);
}

GroupIO::~GroupIO() noexcept
{
  close();
}

hid_t GroupIO::open() const
{
  if(isOpen())
  {
    return getId();
  }
  // Resolve wrapper state before the non-recursive HDF5 lock.
  const hid_t parentId = getParentId();
  const std::string namePath = getNamePath();
  hid_t id = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    id = H5Gopen(parentId, namePath.c_str(), H5P_DEFAULT);
  }
  setId(id);
  return id;
}

void GroupIO::close()
{
  // Resolve the identifier before the non-recursive lock. This also serializes
  // destruction with other HDF5 calls.
  if(isOpen())
  {
    const hid_t selfId = getId();
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      H5Gclose(selfId);
    }
    setId(0);
  }
}

GroupIO GroupIO::openGroup(const std::string& name) const
{
  // Type checks and lazy opens acquire the lock. Complete them before the leaf call.
  if(!isGroup(name))
  {
    std::string ss = fmt::format("Could not open Group '{}'. Child object does not exist or object is not a Group", name);
    std::cout << ss << std::endl;
    return {};
  }
  const hid_t selfId = getId();
  hid_t groupId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    groupId = H5Gopen(selfId, name.c_str(), H5P_DEFAULT);
  }
  if(groupId <= 0)
  {
    std::string ss = fmt::format("Failed to open Group '{}'.", name);
    std::cout << ss << std::endl;
    return {};
  }
  return GroupIO(selfId, name, groupId);
}

DatasetIO GroupIO::openDataset(const std::string& name) const
{
  // The type check can acquire the lock. The returned DatasetIO opens lazily.
  if(!isDataset(name))
  {
    std::string ss = fmt::format("Could not open Dataset '{}'. Child object does not exist or object is not a Dataset", name);
    std::cout << ss << std::endl;
    return {};
  }
  return DatasetIO(getId(), name);
}

usize GroupIO::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  // A lazy open can acquire the lock. Resolve it before the leaf call.
  const hid_t selfId = getId();
  hsize_t numChildren = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    H5Gget_num_objs(selfId, &numChildren);
  }
  return numChildren;
}

std::string GroupIO::getChildNameByIdx(hsize_t idx) const
{
  // A lazy open can acquire the lock. Resolve it before the leaf call.
  const hid_t selfId = getId();
  const size_t size = 1024;
  char buffer[size];
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    H5Gget_objname_by_idx(selfId, idx, buffer, size);
  }
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
  // A lazy open acquires the non-recursive lock. Complete it before the query.
  open();
  if(!isValid())
  {
    return ObjectType::Unknown;
  }

  const hid_t selfId = getId();
  herr_t error = 1;
  H5O_info2_t objectInfo{};
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    HDF_ERROR_HANDLER_OFF
    error = H5Oget_info_by_name3(selfId, childName.c_str(), &objectInfo, H5O_INFO_BASIC, H5P_DEFAULT);
    HDF_ERROR_HANDLER_ON
  }
  if(error < 0)
  {
    return ObjectType::Unknown;
  }

  int32 objectType = objectInfo.type;
  switch(objectType)
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

GroupIO GroupIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current group is not valid", childName);
    std::cout << ss << std::endl;
    return {};
  }
  // Type checks and lazy opens acquire the lock. Complete them before the leaf calls.
  const bool childIsGroup = isGroup(childName);
  const bool childExists = childIsGroup || exists(childName);
  const hid_t selfId = getId();
  hid_t groupId = -1;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    if(childIsGroup)
    {
      groupId = H5Gopen(selfId, childName.c_str(), H5P_DEFAULT);
    }
    else if(!childExists)
    {
      groupId = H5Gcreate(selfId, childName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
  }
  if(groupId > 0)
  {
    return GroupIO(selfId, childName, groupId);
  }

  std::string ss = fmt::format("Failed to create HDF5 group '{}' at path: ", childName, getObjectPath());
  std::cout << ss << std::endl;
  return {};
}

DatasetIO GroupIO::openDataset(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot open Dataset '{}'. Current object is not valid.", childName);
    std::cout << ss << std::endl;
    return {};
  }

  if(isDataset(childName) || !exists(childName))
  {
    return DatasetIO(getId(), childName);
  }

  std::string ss = fmt::format("Failed to open Dataset '{}' at path: ", childName, getObjectPath());
  std::cout << ss << std::endl;
  return {};
}

std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& childName) const
{
  if(!isValid())
  {
    return nullptr;
  }
  if(!isDataset(childName))
  {
    return nullptr;
  }

  return std::make_shared<DatasetIO>(getId(), childName);
}

std::shared_ptr<GroupIO> GroupIO::openGroupPtr(const std::string& name) const
{
  // The type check and lazy open can acquire the lock. Complete them first.
  if(!isGroup(name))
  {
    std::string ss = fmt::format("Could not open Group '{}'. Child object does not exist or object is not a Group", name);
    std::cout << ss << std::endl;
    return nullptr;
  }
  const hid_t selfId = getId();
  hid_t groupId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    groupId = H5Gopen(selfId, name.c_str(), H5P_DEFAULT);
  }
  if(groupId <= 0)
  {
    std::string ss = fmt::format("Failed to open Group '{}'.", name);
    std::cout << ss << std::endl;
    return nullptr;
  }
  return std::shared_ptr<GroupIO>(new GroupIO(selfId, name, groupId));
}

DatasetIO GroupIO::createDataset(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Dataset '{}' as the current Group is not valid.", childName);
    std::cout << ss << std::endl;
    return {};
  }

  return DatasetIO(getId(), childName);
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

  // A lazy open can acquire the lock. Resolve both identifiers before the leaf call.
  const hid_t parentId = getParentId();
  const hid_t selfId = getId();
  herr_t errorCode = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    errorCode = H5Lcreate_hard(parentId, objectPath.c_str(), selfId, objectName.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  }
  if(errorCode < 0)
  {
    return MakeErrorResult(errorCode, fmt::format("Error creating link to path: {}", objectPath));
  }
  return {};
}
} // namespace nx::core::HDF5

#include "GroupIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

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

GroupIO::GroupIO(GroupIO& parentGroup, HighFive::Group&& group, const std::string& groupName)
: ObjectIO(parentGroup, groupName)
, m_Group(std::move(group))
{
}

GroupIO::~GroupIO() noexcept
{
}

HighFive::ObjectType GroupIO::getObjectType() const
{
  return HighFive::ObjectType::Group;
}

Result<GroupIO> GroupIO::openGroup(const std::string& name) const
{
  if(!isGroup(name))
  {
    std::string ss = fmt::format("Could not open Group '{}'. Child object does not exist or object is not a Group", name);
    return MakeErrorResult<GroupIO>(-404, ss);
  }
  auto childGroup = m_Group.getGroup(name);
  return {GroupIO(const_cast<GroupIO&>(*this), std::move(childGroup), name)};
}

std::shared_ptr<GroupIO> GroupIO::openGroupPtr(const std::string& name) const
{
  if(!isGroup(name))
  {
    return nullptr;
  }
  auto childGroup = m_Group.getGroup(name);
  return std::make_shared<GroupIO>(const_cast<GroupIO&>(*this), std::move(childGroup), name);
}

Result<DatasetIO> GroupIO::openDataset(const std::string& name) const
{
  if(!isDataset(name))
  {
    std::string ss = fmt::format("Could not open Dataset '{}'. Child object does not exist or object is not a Dataset", name);
    return MakeErrorResult<DatasetIO>(-405, ss);
  }
  return {DatasetIO(const_cast<GroupIO&>(*this), name)};
}

std::shared_ptr<DatasetIO> GroupIO::openDatasetPtr(const std::string& name) const
{
  if(!isDataset(name))
  {
    return nullptr;
  }
  return std::make_shared<DatasetIO>(const_cast<GroupIO&>(*this), name);
}

usize GroupIO::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  return m_Group.getNumberObjects();
}

std::vector<std::string> GroupIO::getChildNames() const
{
  if(!isValid())
  {
    return {};
  }

  const usize numChildren = m_Group.getNumberObjects();

  std::vector<std::string> childNames(numChildren);
  for(usize i = 0; i < numChildren; i++)
  {
    childNames[i] = m_Group.getObjectName(i);
  }

  return childNames;
}

bool GroupIO::isGroup(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  if(!m_Group.exist(childName))
  {
    return false;
  }
  return m_Group.getObjectType(childName) == HighFive::ObjectType::Group;
}

bool GroupIO::isDataset(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  if(!m_Group.exist(childName))
  {
    return false;
  }
  return m_Group.getObjectType(childName) == HighFive::ObjectType::Dataset;
}

Result<GroupIO> GroupIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current group is not valid", childName);
    return MakeErrorResult<GroupIO>(-505, ss);
  }
  if(m_Group.exist(childName))
  {
    if(isGroup(childName))
    {
      auto childGroup = m_Group.getGroup(childName);
      return {GroupIO(*this, std::move(childGroup), childName)};
    }
    else
    {
      std::string ss = fmt::format("Cannot create Group '{}' as an child object with that name already exists but is not the correct type.", childName);
      return MakeErrorResult<GroupIO>(-604, ss);
    }
  }
  auto childGroup = m_Group.createGroup(childName);
  return {GroupIO(*this, std::move(childGroup), childName)};
}

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

Result<DatasetIO> GroupIO::openDataset(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot open Dataset '{}'. Current object is not valid.", childName);
    return MakeErrorResult<DatasetIO>(-406, ss);
  }
  if(!isDataset(childName))
  {
    std::string ss = fmt::format("Cannot open Dataset '{}'. Child Dataset does not exist.", childName);
    return MakeErrorResult<DatasetIO>(-606, ss);
  }

  return {DatasetIO(*this, childName)};
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

usize GroupIO::getNumAttributes() const
{
  return m_Group.getNumberAttributes();
}

std::vector<std::string> GroupIO::getAttributeNames() const
{
  return m_Group.listAttributeNames();
}

void GroupIO::deleteAttribute(const std::string& name)
{
  m_Group.deleteAttribute(name);
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

  m_Group.createSoftLink(objectName, objectPath);
  return {};
}

HighFive::Group& GroupIO::groupRef()
{
  return m_Group;
}

const HighFive::Group& GroupIO::groupRef() const
{
  return m_Group;
}

HighFive::DataSet GroupIO::openH5Dataset(const std::string& name) const
{
  try
  {
    return m_Group.getDataSet(name);
  }
  catch(const std::exception& e)
  {
    throw e;
  }
}

HighFive::DataSet GroupIO::createOrOpenH5Dataset(const std::string& name, const HighFive::DataSpace& dims, HighFive::DataType dataType)
{
  try
  {
    if(m_Group.exist(name))
    {
      return m_Group.getDataSet(name);
    }
    else
    {
      return m_Group.createDataSet(name, dims, dataType);
    }
  } catch(const std::exception& e)
  {
    throw e;
  }
}

hid_t GroupIO::createOrOpenHDF5Dataset(const std::string& name, hid_t typeId, hid_t dataspaceId, hid_t propertiesId)
{
  hid_t parentId = getH5Id();

  HDF_ERROR_HANDLER_OFF
  hid_t id = (H5Dopen(parentId, getName().c_str(), H5P_DEFAULT));
  HDF_ERROR_HANDLER_ON
  if(id < 0) // dataset does not exist so create it
  {
    id = (H5Dcreate(parentId, getName().c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
  }
  return id;
}

hid_t GroupIO::getH5Id() const
{
  return m_Group.getId();
}
// -----------------------------------------------------------------------------
} // namespace nx::core::HDF5

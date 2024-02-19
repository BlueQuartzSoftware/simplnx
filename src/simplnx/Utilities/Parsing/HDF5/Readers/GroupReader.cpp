#include "GroupReader.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Gpublic.h>
#include <H5Opublic.h>

#include <iostream>

namespace nx::core::HDF5
{
GroupReader::GroupReader() = default;

GroupReader::GroupReader(IdType parentId, const std::string& groupName)
: ObjectReader(parentId, H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT))
{
}

GroupReader::GroupReader(IdType parentId, IdType objectId)
: ObjectReader(parentId, objectId)
{
}

GroupReader::~GroupReader() noexcept
{
  closeHdf5();
}

void GroupReader::closeHdf5()
{
  if(isValid())
  {
    H5Gclose(getId());
    setId(0);
  }
}

GroupReader GroupReader::openGroup(const std::string& name) const
{
  if(!isValid())
  {
    return GroupReader();
  }

  return GroupReader(getId(), name);
}

DatasetReader GroupReader::openDataset(const std::string& name) const
{
  if(!isValid())
  {
    return DatasetReader();
  }

  return DatasetReader(getId(), name);
}

ObjectReader GroupReader::openObject(const std::string& name) const
{
  if(!isValid())
  {
    return ObjectReader();
  }

  return ObjectReader(getId(), name);
}

size_t GroupReader::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  hsize_t numChildren;
  auto err = H5Gget_num_objs(getId(), &numChildren);
  if(err < 0)
  {
    return 0;
  }
  return static_cast<size_t>(numChildren);
}

std::vector<std::string> GroupReader::getChildNames() const
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

bool GroupReader::isGroup(const std::string& childName) const
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

bool GroupReader::isDataset(const std::string& childName) const
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
} // namespace nx::core::HDF5

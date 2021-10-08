#include "H5GroupReader.hpp"

#include <iostream>

#include <H5Gpublic.h>
#include <H5Opublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::GroupReader::GroupReader() = default;

H5::GroupReader::GroupReader(H5::IdType parentId, const std::string& groupName)
: ObjectReader(parentId)
{
  m_GroupId = H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT);
}

H5::GroupReader::~GroupReader() = default;

void H5::GroupReader::closeHdf5()
{
  if(isValid())
  {
    H5Gclose(m_GroupId);
    m_GroupId = 0;
  }
}

H5::IdType H5::GroupReader::getId() const
{
  return m_GroupId;
}

H5::GroupReader H5::GroupReader::openGroup(const std::string& name) const
{
  if(!isValid())
  {
    return GroupReader();
  }

  return GroupReader(getId(), name);
}

H5::DatasetReader H5::GroupReader::openDataset(const std::string& name) const
{
  if(!isValid())
  {
    return DatasetReader();
  }

  return DatasetReader(getId(), name);
}

H5::ObjectReader H5::GroupReader::openObject(const std::string& name) const
{
  if(!isValid())
  {
    return ObjectReader();
  }

  return ObjectReader(getId(), name);
}

size_t H5::GroupReader::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  H5::SizeType numChildren;
  auto err = H5Gget_num_objs(getId(), &numChildren);
  if(err < 0)
  {
    return 0;
  }
  return numChildren;
}

std::vector<std::string> H5::GroupReader::getChildNames() const
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

bool H5::GroupReader::isGroup(const std::string& childName) const
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

bool H5::GroupReader::isDataset(const std::string& childName) const
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

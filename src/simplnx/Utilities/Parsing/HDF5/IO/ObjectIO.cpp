#include "ObjectIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <H5Apublic.h>

namespace nx::core::HDF5
{
ObjectIO::ObjectIO() = default;

ObjectIO::ObjectIO(IdType parentId)
: m_ParentId(parentId)
{
}

ObjectIO::ObjectIO(IdType parentId, IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

ObjectIO::ObjectIO(IdType parentId, const std::string& targetName)
: m_ParentId(parentId)
{
  m_Id = H5Oopen(parentId, targetName.c_str(), H5P_DEFAULT);
}

ObjectIO::ObjectIO(ObjectIO&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
  m_SharedParentPtr = std::move(other.m_SharedParentPtr);
}

ObjectIO& ObjectIO::operator=(ObjectIO&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
  m_SharedParentPtr = std::move(other.m_SharedParentPtr);
  return *this;
}

ObjectIO::~ObjectIO() noexcept
{
  closeHdf5();
}

void ObjectIO::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(m_Id);
    m_Id = 0;
  }
}

void ObjectIO::clear()
{
  m_Id = 0;
  m_ParentId = 0;
}

bool ObjectIO::isValid() const
{
  return getId() > 0;
}

IdType ObjectIO::getFileId() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Iget_file_id(getParentId());
}

IdType ObjectIO::getParentId() const
{
  return m_ParentId;
}

void ObjectIO::setParentId(IdType parentId)
{
  m_ParentId = parentId;
  m_SharedParentPtr = nullptr;
}

void ObjectIO::setSharedParent(std::shared_ptr<GroupIO> sharedParent)
{
  if(sharedParent == nullptr)
  {
    setParentId(0);
  }
  else
  {
    setParentId(sharedParent->getId());
    m_SharedParentPtr = sharedParent;
  }
}

haddr_t ObjectIO::getObjectId() const
{
  if(!isValid())
  {
    return 0;
  }

  H5O_info1_t info;
  H5Oget_info(m_Id, &info);
  return info.addr;
}

IdType ObjectIO::getId() const
{
  return m_Id;
}

void ObjectIO::setId(IdType identifier)
{
  m_Id = identifier;
}

std::string ObjectIO::getName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getId());
  return path;
}

std::string ObjectIO::getParentName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getParentId());

  return path;
}

std::string ObjectIO::getObjectPath() const
{
  if(!isValid())
  {
    return "";
  }

  return Support::GetObjectPath(getId());
}

size_t ObjectIO::getNumAttributes() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Aget_num_attrs(getId());
}

std::vector<std::string> ObjectIO::getAttributeNames() const
{
  auto numAttributes = getNumAttributes();
  std::vector<std::string> attributeNames(numAttributes);
  for(size_t i = 0; i < numAttributes; i++)
  {
    attributeNames[i] = getAttributeByIdx(i).getName();
  }

  return attributeNames;
}

AttributeIO ObjectIO::getAttribute(const std::string& name) const
{
  if(!isValid())
  {
    return AttributeIO();
  }

  return AttributeIO(getId(), name);
}

AttributeIO ObjectIO::getAttributeByIdx(size_t idx) const
{
  if(!isValid())
  {
    return AttributeIO();
  }

  return AttributeIO(getId(), idx);
}

AttributeIO ObjectIO::createAttribute(const std::string& name)
{
  if(!isValid())
  {
    return AttributeIO();
  }

  return AttributeIO(getId(), name);
}
} // namespace nx::core::HDF5

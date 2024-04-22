#include "ObjectReader.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

namespace nx::core::HDF5
{
ObjectReader::ObjectReader() = default;

ObjectReader::ObjectReader(IdType parentId)
: m_ParentId(parentId)
{
}

ObjectReader::ObjectReader(IdType parentId, IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

ObjectReader::ObjectReader(IdType parentId, const std::string& targetName)
: m_ParentId(parentId)
{
  m_Id = H5Oopen(parentId, targetName.c_str(), H5P_DEFAULT);
}

ObjectReader::ObjectReader(ObjectReader&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
}

ObjectReader& ObjectReader::operator=(ObjectReader&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
  return *this;
}

ObjectReader::~ObjectReader() noexcept
{
  closeHdf5();
}

void ObjectReader::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(m_Id);
    m_Id = 0;
  }
}

bool ObjectReader::isValid() const
{
  return getId() > 0;
}

IdType ObjectReader::getParentId() const
{
  return m_ParentId;
}

haddr_t ObjectReader::getObjectId() const
{
  if(!isValid())
  {
    return 0;
  }

  H5O_info1_t info;
  H5Oget_info(m_Id, &info);
  return info.addr;
}

IdType ObjectReader::getId() const
{
  return m_Id;
}

void ObjectReader::setId(IdType identifier)
{
  m_Id = identifier;
}

std::string ObjectReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getId());

  return path;
}

size_t ObjectReader::getNumAttributes() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Aget_num_attrs(getId());
}

std::vector<std::string> ObjectReader::getAttributeNames() const
{
  auto numAttributes = getNumAttributes();
  std::vector<std::string> attributeNames(numAttributes);
  for(size_t i = 0; i < numAttributes; i++)
  {
    attributeNames[i] = getAttributeByIdx(i).getName();
  }

  return attributeNames;
}

AttributeReader ObjectReader::getAttribute(const std::string& name) const
{
  if(!isValid())
  {
    return AttributeReader();
  }

  return AttributeReader(getId(), name);
}

AttributeReader ObjectReader::getAttributeByIdx(size_t idx) const
{
  if(!isValid())
  {
    return AttributeReader();
  }

  return AttributeReader(getId(), idx);
}
} // namespace nx::core::HDF5

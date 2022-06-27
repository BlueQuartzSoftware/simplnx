#include "H5ObjectReader.hpp"

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::ObjectReader::ObjectReader() = default;

H5::ObjectReader::ObjectReader(H5::IdType parentId)
: m_ParentId(parentId)
{
}

H5::ObjectReader::ObjectReader(H5::IdType parentId, H5::IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

H5::ObjectReader::ObjectReader(H5::IdType parentId, const std::string& targetName)
: m_ParentId(parentId)
{
  m_Id = H5Oopen(parentId, targetName.c_str(), H5P_DEFAULT);
}

H5::ObjectReader::~ObjectReader()
{
  closeHdf5();
}

void H5::ObjectReader::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(m_Id);
    m_Id = 0;
  }
}

bool H5::ObjectReader::isValid() const
{
  return getId() > 0;
}

H5::IdType H5::ObjectReader::getParentId() const
{
  return m_ParentId;
}

size_t H5::ObjectReader::getObjectId() const
{
  if(!isValid())
  {
    return 0;
  }

  H5O_info1_t info;
  H5Oget_info(m_Id, &info);
  return info.addr;
}

H5::IdType H5::ObjectReader::getId() const
{
  return m_Id;
}

void H5::ObjectReader::setId(H5::IdType id)
{
  m_Id = id;
}

std::string H5::ObjectReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = H5::GetNameFromId(getId());

  return path;
}

size_t H5::ObjectReader::getNumAttributes() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Aget_num_attrs(getId());
}

std::vector<std::string> H5::ObjectReader::getAttributeNames() const
{
  auto numAttributes = getNumAttributes();
  std::vector<std::string> attributeNames(numAttributes);
  for(size_t i = 0; i < numAttributes; i++)
  {
    attributeNames[i] = getAttributeByIdx(i).getName();
  }

  return attributeNames;
}

H5::AttributeReader H5::ObjectReader::getAttribute(const std::string& name) const
{
  if(!isValid())
  {
    return AttributeReader();
  }

  return AttributeReader(getId(), name);
}

H5::AttributeReader H5::ObjectReader::getAttributeByIdx(size_t idx) const
{
  if(!isValid())
  {
    return AttributeReader();
  }

  return AttributeReader(getId(), idx);
}

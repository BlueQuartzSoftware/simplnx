#include "H5ObjectReader.hpp"

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::ObjectReader::ObjectReader() = default;

H5::ObjectReader::ObjectReader(H5::IdType parentId)
: m_ParentId(parentId)
{
}

H5::ObjectReader::ObjectReader(H5::IdType parentId, const std::string& targetName)
: m_ParentId(parentId)
{
  m_ObjectId = H5Oopen(parentId, targetName.c_str(), H5P_DEFAULT);
}

H5::ObjectReader::~ObjectReader()
{
  closeHdf5();
}

void H5::ObjectReader::closeHdf5()
{
  if(isValid())
  {
    H5Oclose(m_ObjectId);
    m_ObjectId = 0;
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

H5::IdType H5::ObjectReader::getId() const
{
  return m_ObjectId;
}

std::string H5::ObjectReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 1024;
  char buffer[size];
  H5Iget_name(getId(), buffer, size);

  return GetNameFromBuffer(buffer);
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
  std::vector<std::string> attributeNames;
  auto numAttributes = getNumAttributes();
  for(size_t i = 0; i < numAttributes; i++)
  {
    getAttributeByIdx(i);
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

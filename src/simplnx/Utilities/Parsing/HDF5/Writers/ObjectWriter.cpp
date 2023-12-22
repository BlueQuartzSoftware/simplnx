#include "ObjectWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

namespace nx::core::HDF5
{

ObjectWriter::ObjectWriter()
{
}

ObjectWriter::ObjectWriter(IdType parentId, IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

ObjectWriter::~ObjectWriter() = default;

IdType ObjectWriter::getFileId() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Iget_file_id(getParentId());
}

IdType ObjectWriter::getParentId() const
{
  return m_ParentId;
}

std::string ObjectWriter::getParentName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getParentId());

  return path;
}

IdType ObjectWriter::getId() const
{
  return m_Id;
}

void ObjectWriter::setId(IdType identifier)
{
  m_Id = identifier;
}

std::string ObjectWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = GetNameFromId(getId());

  return path;
}

std::string ObjectWriter::getObjectPath() const
{
  if(!isValid())
  {
    return "";
  }

  return Support::GetObjectPath(getId());
}

AttributeWriter ObjectWriter::createAttribute(const std::string& name)
{
  if(!isValid())
  {
    return AttributeWriter();
  }

  return AttributeWriter(getId(), name);
}
} // namespace nx::core::HDF5

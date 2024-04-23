#include "ObjectWriter.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

namespace nx::core::HDF5
{

ObjectWriter::ObjectWriter() = default;

ObjectWriter::ObjectWriter(IdType parentId, IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

ObjectWriter::ObjectWriter(ObjectWriter&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
}

ObjectWriter& ObjectWriter::operator=(ObjectWriter&& other) noexcept
{
  m_Id = std::exchange(other.m_Id, 0);
  m_ParentId = std::exchange(other.m_ParentId, 0);
  return *this;
}

ObjectWriter::~ObjectWriter() noexcept = default;

IdType ObjectWriter::getFileId() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Iget_file_id(getParentId());
}

void ObjectWriter::setParentId(IdType parentId)
{
  m_ParentId = parentId;
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

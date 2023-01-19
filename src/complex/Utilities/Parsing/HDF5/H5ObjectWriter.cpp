#include "H5ObjectWriter.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <H5Apublic.h>

using namespace complex;

H5::ObjectWriter::ObjectWriter()
{
}

H5::ObjectWriter::ObjectWriter(H5::IdType parentId, H5::IdType objectId)
: m_ParentId(parentId)
, m_Id(objectId)
{
}

H5::ObjectWriter::~ObjectWriter() = default;

H5::IdType H5::ObjectWriter::getFileId() const
{
  if(!isValid())
  {
    return 0;
  }

  return H5Iget_file_id(getParentId());
}

H5::IdType H5::ObjectWriter::getParentId() const
{
  return m_ParentId;
}

std::string H5::ObjectWriter::getParentName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = H5::GetNameFromId(getParentId());

  return path;
}

H5::IdType H5::ObjectWriter::getId() const
{
  return m_Id;
}

void H5::ObjectWriter::setId(H5::IdType identifier)
{
  m_Id = identifier;
}

std::string H5::ObjectWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  std::string path = H5::GetNameFromId(getId());

  return path;
}

std::string H5::ObjectWriter::getObjectPath() const
{
  if(!isValid())
  {
    return "";
  }

  return H5::Support::GetObjectPath(getId());
}

H5::AttributeWriter H5::ObjectWriter::createAttribute(const std::string& name)
{
  if(!isValid())
  {
    return AttributeWriter();
  }

  return AttributeWriter(getId(), name);
}

#include "H5ObjectWriter.hpp"

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::ObjectWriter::ObjectWriter()
{
}

H5::ObjectWriter::ObjectWriter(H5::IdType parentId)
: m_ParentId(parentId)
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

  constexpr size_t size = 1024;
  char buffer[size];
  H5Iget_name(getParentId(), buffer, size);

  return H5::GetNameFromBuffer(buffer);
}

std::string H5::ObjectWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 1024;
  char buffer[size];
  H5Iget_name(getId(), buffer, size);

  return H5::GetNameFromBuffer(buffer);
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

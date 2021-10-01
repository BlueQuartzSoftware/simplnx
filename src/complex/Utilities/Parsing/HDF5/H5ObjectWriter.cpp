#include "H5ObjectWriter.hpp"

#include <H5Apublic.h>

using namespace complex;

H5::ObjectWriter::ObjectWriter()
{
}

H5::ObjectWriter::ObjectWriter(H5::IdType parentId)
: m_ParentId(parentId)
{
}

H5::ObjectWriter::~ObjectWriter() = default;

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

  constexpr size_t size = 256;
  char buffer[size];
  H5Iget_name(getParentId(), buffer, size);

  return std::string(buffer);
}

std::string H5::ObjectWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 256;
  char buffer[size];
  H5Iget_name(getId(), buffer, size);

  return std::string(buffer);
}

H5::AttributeWriter H5::ObjectWriter::createAttribute(const std::string& name)
{
  if(!isValid())
  {
    return AttributeWriter();
  }

  return AttributeWriter(getId(), name);
}

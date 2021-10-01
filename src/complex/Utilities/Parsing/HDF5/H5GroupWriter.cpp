#include "H5GroupWriter.hpp"

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::GroupWriter::GroupWriter()
: ObjectWriter()
{
}

H5::GroupWriter::GroupWriter(H5::IdType parentId, const std::string& groupName)
: ObjectWriter(parentId)
{
  m_GroupId = H5Gopen(parentId, groupName.c_str(), H5P_DEFAULT);
  if(m_GroupId == 0)
  {
    m_GroupId = H5Gcreate(parentId, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
}

H5::GroupWriter::~GroupWriter()
{
  closeHdf5();
}

void H5::GroupWriter::closeHdf5()
{
  H5Oclose(m_GroupId);
  m_GroupId = 0;
}

bool H5::GroupWriter::isValid() const
{
  return getId() != 0;
}

H5::IdType H5::GroupWriter::getId() const
{
  return m_GroupId;
}

H5::GroupWriter H5::GroupWriter::createGroupWriter(const std::string& childName)
{
  if(!isValid())
  {
    return GroupWriter();
  }

  return GroupWriter(getId(), childName);
}

H5::DatasetWriter H5::GroupWriter::createDatasetWriter(const std::string& childName)
{
  if(!isValid())
  {
    return DatasetWriter();
  }

  return DatasetWriter(getId(), childName);
}

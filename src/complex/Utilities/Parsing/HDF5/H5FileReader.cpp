#include "H5FileReader.hpp"

#include <H5Apublic.h>

using namespace complex;

H5::FileReader::FileReader(const std::filesystem::path& filepath)
: GroupReader()
{
  m_FileId = H5Fopen(filepath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
}

H5::FileReader::FileReader(H5::IdType fileId)
: GroupReader()
, m_FileId(fileId)
{
}

H5::FileReader::~FileReader() = default;

void H5::FileReader::closeHdf5()
{
  if(isValid())
  {
    H5Fclose(m_FileId);
    m_FileId = 0;
  }
}

H5::IdType H5::FileReader::getId() const
{
  return m_FileId;
}

std::string H5::FileReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 1024;
  char buffer[size];
  H5Fget_name(getId(), buffer, size);

  return H5::GetNameFromBuffer(buffer);
}

#include "H5FileWriter.hpp"

using namespace complex;

H5::FileWriter::FileWriter()
: GroupWriter()
{
}

H5::FileWriter::FileWriter(const std::filesystem::path& filepath)
: GroupWriter()
{
  m_FileId = H5Fcreate(filepath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
}

H5::FileWriter::FileWriter(H5::IdType fileId)
: GroupWriter()
, m_FileId(fileId)
{
}

H5::FileWriter::~FileWriter()
{
  closeHdf5();
}

void H5::FileWriter::closeHdf5()
{
  if(isValid())
  {
    H5Fclose(m_FileId);
    m_FileId = 0;
  }
}

H5::IdType H5::FileWriter::getId() const
{
  return m_FileId;
}

std::string H5::FileWriter::getName() const
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

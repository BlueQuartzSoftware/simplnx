#include "H5FileWriter.hpp"

#include <fmt/format.h>

#include <stdexcept>

using namespace complex;

Result<H5::FileWriter> H5::FileWriter::CreateFile(const std::filesystem::path& filepath)
{
  Result<H5::FileWriter> result;

  auto parentPath = filepath.parent_path();
  if(!std::filesystem::exists(parentPath))
  {
    if(!std::filesystem::create_directories(parentPath))
    {
      return MakeErrorResult<H5::FileWriter>(-300, fmt::format("Error creating Output HDF5 file at path '{}'. Parent path could not be created.", filepath.string()));
    }
  }

  try
  {
    return {FileWriter(filepath)};
  } catch(const std::runtime_error& error)
  {
    return MakeErrorResult<H5::FileWriter>(-301, fmt::format("Error creating Output HDF5 file at path '{}'. Parent path could not be created.", filepath.string()));
  }
  return {}; // Code should not get here. We return everywhere else.
}

Result<H5::FileWriter> H5::FileWriter::WrapHdf5FileId(H5::IdType fileId)
{
  if(fileId <= 0)
  {
    return MakeErrorResult<H5::FileWriter>(-302, fmt::format("Error wrapping existing HDF5 FileId with value '{}'.", fileId));
  }
  return {FileWriter(fileId)};
}

H5::FileWriter::FileWriter() = default;

H5::FileWriter::FileWriter(FileWriter&& rhs) noexcept
: GroupWriter()
, m_FileId(std::exchange(rhs.m_FileId, -1))
{
}

H5::FileWriter::FileWriter(const std::filesystem::path& filepath)
{
  m_FileId = H5Fcreate(filepath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if(m_FileId < 0)
  {
    throw std::runtime_error(fmt::format("Error creating Output HDF5 file at path '{}'. HDF5Library threw error.", filepath.string()));
  }
}

H5::FileWriter::FileWriter(H5::IdType fileId)
: m_FileId(fileId)
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

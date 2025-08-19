#include "FileIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <fmt/format.h>

namespace nx::core::HDF5
{
FileIO FileIO::ReadFile(const std::filesystem::path& filepath)
{
  hid_t fileId = H5Fopen(filepath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  return FileIO(filepath, fileId);
}

FileIO FileIO::WriteFile(const std::filesystem::path& filepath)
{
  if(std::filesystem::exists(filepath))
  {
    try
    {
      std::filesystem::remove(filepath);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Failed to remove file at path '{}'. Error: '{}'", filepath.string(), e.what());
      std::cout << msg << std::endl;
    }
  }

  hid_t fileId = H5Fcreate(filepath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if(fileId > 0)
  {
    return FileIO(filepath, fileId);
  }
  return {};
}

FileIO FileIO::AppendFile(const std::filesystem::path& filepath)
{
  hid_t fileId = H5Fopen(filepath.string().c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  return FileIO(filepath, fileId);
}

FileIO::FileIO(const std::filesystem::path& filepath, hid_t fileId)
: GroupIO()
{
  setFilePath(filepath);
  setId(fileId);
}

FileIO::~FileIO() noexcept
{
  close();
}

hid_t FileIO::open() const
{
  if(isOpen())
  {
    return getId();
  }
  hid_t id = H5Fopen(getFilePath().string().c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  setId(id);
  return id;
}

void FileIO::close()
{
  if(isOpen())
  {
    H5Fclose(getId());
    setId(-1);
  }
}

std::string FileIO::getName() const
{
  if(!isValid())
  {
    return "";
  }
  return getFilePath().filename().string();
}

std::string FileIO::getNamePath() const
{
  return "";
}

std::string FileIO::getObjectPath() const
{
  return "";
}

} // namespace nx::core::HDF5

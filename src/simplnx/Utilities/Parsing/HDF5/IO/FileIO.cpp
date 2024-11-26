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
  if (fileId > 0)
  {
    return FileIO(filepath, fileId);
  }
  return {};
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
    setId(0);
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

std::string FileIO::getObjectPath() const
{
  return "";
}

#if 0
usize FileIO::getNumAttributes() const
{
  auto file = HighFive::File(getFilePath().string(), HighFive::File::ReadOnly);
  return file.getNumberAttributes();
}

std::vector<std::string> FileIO::getAttributeNames() const
{
  auto file = HighFive::File(getFilePath().string(), HighFive::File::ReadOnly);
  return file.listAttributeNames();
}

void FileIO::deleteAttribute(const std::string& name)
{
  auto file = HighFive::File(getFilePath().string(), HighFive::File::ReadWrite);
  file.deleteAttribute(name);
}
#endif

#if 0
bool FileIO::isGroup(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  if(!m_File->exist(childName))
  {
    return false;
  }
  return m_File->getObjectType(childName) == ObjectType::Group;
}

bool FileIO::isDataset(const std::string& childName) const
{
  if(!isValid())
  {
    return false;
  }

  if(!m_File->exist(childName))
  {
    return false;
  }
  return m_File->getObjectType(childName) == ObjectType::Dataset;
}
#endif

#if 0
Result<GroupIO> FileIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current HDF5 FileIO is not valid.", childName);
    return MakeErrorResult<GroupIO>(-704, ss);
  }
  hid_t groupId = -1;
  if(isGroup(childName))
  {
    groupId = H5Gopen(getId(), childName.c_str(), H5P_DEFAULT);
  }
  else if (!exists(childName))
  {
    groupId = H5Gcreate(getId(), childName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
  if(groupId > 0)
  {
    return {GroupIO(*this, childName, groupId)};
  }

  std::string ss = fmt::format("Failed to create HDF5 group '{}' at path: ", childName, getObjectPath());
  return MakeErrorResult<GroupIO>(-722, ss);
}

std::shared_ptr<GroupIO> FileIO::createGroupPtr(const std::string& childName)
{
  if(!isValid())
  {
    return nullptr;
  }
  auto childGroup = m_File->createGroup(childName);
  return std::make_shared<GroupIO>(*this, std::move(childGroup), childName);
}
#endif

#if 0
Result<GroupIO> FileIO::openGroup(const std::string& name) const
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current HDF5 FileIO is not valid.", name);
    return MakeErrorResult<GroupIO>(-804, ss);
  }
  if(!m_File->exist(name))
  {
    std::string ss = fmt::format("Cannot create Group '{}' as a child of that name does not exist.", name);
    return MakeErrorResult<GroupIO>(-805, ss);
  }
  if(m_File->exist(name) && !isGroup(name))
  {
    std::string ss = fmt::format("Cannot create Group '{}' as a child of that name already exists but is not the correct type.", name);
    return MakeErrorResult<GroupIO>(-806, ss);
  }

  auto childGroup = m_File->getGroup(name);
  return {GroupIO(const_cast<FileIO&>(*this), std::move(childGroup), name)};
}
#endif

#if 0
std::shared_ptr<GroupIO> FileIO::openGroupPtr(const std::string& name) const
{
  auto childGroup = m_File->getGroup(name);
  return std::make_shared<GroupIO>(const_cast<FileIO&>(*this), std::move(childGroup), name);
}
#endif
} // namespace nx::core::HDF5

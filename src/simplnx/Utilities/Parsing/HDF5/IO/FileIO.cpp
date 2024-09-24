#include "FileIO.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <fmt/format.h>

namespace nx::core::HDF5
{
FileIO FileIO::ReadFile(const std::filesystem::path& filepath)
{
  HighFive::File file(filepath.string(), HighFive::File::ReadOnly);
  return FileIO(filepath, std::move(file));
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
  HighFive::File file(filepath.string(), HighFive::File::ReadWrite | HighFive::File::Truncate | HighFive::File::OpenOrCreate);
  return FileIO(filepath, std::move(file));
}

FileIO::FileIO(const std::filesystem::path& filepath, HighFive::File&& file)
: GroupIO()
, m_File(std::move(file))
{
  setFilePath(filepath);
}

FileIO::~FileIO() noexcept
{
}

HighFive::ObjectType FileIO::getObjectType() const
{
  return HighFive::ObjectType::File;
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

usize FileIO::getNumChildren() const
{
  if(!isValid())
  {
    return 0;
  }

  return m_File->getNumberObjects();
}

std::vector<std::string> FileIO::getChildNames() const
{
  if(!isValid())
  {
    return {};
  }

  const usize numChildren = m_File->getNumberObjects();

  std::vector<std::string> childNames(numChildren);
  for(usize i = 0; i < numChildren; i++)
  {
    childNames[i] = m_File->getObjectName(i);
  }

  return childNames;
}

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
  return m_File->getObjectType(childName) == HighFive::ObjectType::Group;
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
  return m_File->getObjectType(childName) == HighFive::ObjectType::Dataset;
}

Result<GroupIO> FileIO::createGroup(const std::string& childName)
{
  if(!isValid())
  {
    std::string ss = fmt::format("Cannot create Group '{}' as the current HDF5 FileIO is not valid.", childName);
    return MakeErrorResult<GroupIO>(-704, ss);
  }
  if(m_File->exist(childName))
  {
    std::string ss = fmt::format("Cannot create Group '{}' as a child of that name already exists.", childName);
    return MakeErrorResult<GroupIO>(-705, ss);
  }
  auto childGroup = m_File->createGroup(childName);
  return {GroupIO(*this, std::move(childGroup), childName)};
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

std::shared_ptr<GroupIO> FileIO::openGroupPtr(const std::string& name) const
{
  auto childGroup = m_File->getGroup(name);
  return std::make_shared<GroupIO>(const_cast<FileIO&>(*this), std::move(childGroup), name);
}

std::optional<HighFive::File> FileIO::h5File() const
{
  return m_File;
}

HighFive::DataSet FileIO::openH5Dataset(const std::string& name) const
{
  if(!m_File.has_value())
  {
    std::string ss = fmt::format("FileIO cannot open DataSet '{}' because the HDF5 file is not available.", name);
    throw std::runtime_error(ss);
  }

  try
  {
    return m_File.value().getDataSet(name);
  } catch(const std::exception& e)
  {
    throw e;
  }
}

HighFive::DataSet FileIO::createOrOpenH5Dataset(const std::string& name, const HighFive::DataSpace& dims, HighFive::DataType dataType)
{
  if(!m_File.has_value())
  {
    std::string ss = fmt::format("FileIO cannot open or create DataSet '{}' because the HDF5 file is not available.", name);
    throw std::runtime_error(ss);
  }

  try
  {
    if(m_File.value().exist(name))
    {
      return m_File.value().getDataSet(name);
    }
    else
    {
      return m_File.value().createDataSet(name, dims, dataType);
    }
  } catch(const std::exception& e)
  {
    throw e;
  }
}

hid_t FileIO::getH5Id() const
{
  if(!m_File.has_value())
  {
    return 0;
  }

  return m_File.value().getId();
}
} // namespace nx::core::HDF5

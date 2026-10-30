#include "FileIO.hpp"

#include "simplnx/Common/PathText.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include <fmt/format.h>

#include <H5Ipublic.h>
#include <H5Ppublic.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <system_error>

namespace
{
// Tests use this process-wide count to verify cached preflight paths without
// timing assertions. ReadFile() can run on preflight worker threads.
std::atomic<nx::core::uint64> s_ReadOpenCount{0};

// Tests can configure a file-access property list, for example to simulate I/O
// latency or a failing close. Production leaves this hook empty and uses H5P_DEFAULT.
std::function<void(hid_t)> s_FaplConfigurator;

/**
 * @brief Creates and configures a file-access property list when tests install a hook.
 * @return H5P_DEFAULT when no hook exists. Otherwise, returns an identifier that the caller must close.
 * @pre The caller holds Support::ApiLock(). The hook can call raw HDF5 functions, but it must not call a wrapper that acquires this non-recursive lock.
 */
hid_t MakeFaplId()
{
  if(!s_FaplConfigurator)
  {
    return H5P_DEFAULT;
  }
  hid_t faplId = H5Pcreate(H5P_FILE_ACCESS);
  s_FaplConfigurator(faplId);
  return faplId;
}

/**
 * @brief Closes a file-access property list that MakeFaplId() created.
 * @param faplId Identifies the property list. H5P_DEFAULT does not require a close operation.
 * @pre The caller holds Support::ApiLock().
 */
void CloseFaplId(hid_t faplId)
{
  if(faplId != H5P_DEFAULT)
  {
    H5Pclose(faplId);
  }
}

/**
 * @brief Describes what else keeps the physical file of fileId open.
 * @param fileId Open file identifier that is about to be closed.
 * @return Empty when fileId is the only holder: its reference count is 1, no other file identifier of the same physical file is open,
 * and no dataset, group, named datatype or attribute of the file is open through any identifier. Otherwise, the cause.
 * @pre The caller holds Support::ApiLock().
 *
 * The counts omit H5F_OBJ_LOCAL on purpose: they must cover every identifier of the same physical file, because with the weak close
 * degree any of them defers the real close of fileId.
 */
std::string DescribeOtherFileHolders(hid_t fileId)
{
  const int referenceCount = H5Iget_ref(fileId);
  const ssize_t fileIdCount = H5Fget_obj_count(fileId, H5F_OBJ_FILE);
  const ssize_t childCount = H5Fget_obj_count(fileId, H5F_OBJ_DATASET | H5F_OBJ_GROUP | H5F_OBJ_DATATYPE | H5F_OBJ_ATTR);
  if(referenceCount < 0 || fileIdCount < 0 || childCount < 0)
  {
    return "its open HDF5 objects could not be counted";
  }
  if(referenceCount != 1)
  {
    return fmt::format("its file identifier holds {} references instead of 1", referenceCount);
  }
  if(fileIdCount != 1)
  {
    return "another identifier of this file is open";
  }
  if(childCount != 0)
  {
    return fmt::format("{} HDF5 objects are still open", childCount);
  }
  return {};
}
} // namespace
namespace nx::core::HDF5
{
FileIO FileIO::ReadFile(const std::filesystem::path& filepath)
{
  s_ReadOpenCount++;
  // The narrow name can throw on Windows, so it is built before the property list exists. Keep the property-list lifecycle and file
  // open in one HDF5 critical section.
  const std::string pathText = filepath.string();
  hid_t fileId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t faplId = MakeFaplId();
    fileId = H5Fopen(pathText.c_str(), H5F_ACC_RDONLY, faplId);
    CloseFaplId(faplId);
  }
  return FileIO(filepath, fileId);
}

uint64 FileIO::GetReadOpenCount()
{
  return s_ReadOpenCount.load();
}

void FileIO::ResetReadOpenCount()
{
  s_ReadOpenCount.store(0);
}

void FileIO::SetFaplConfigurator(std::function<void(hid_t)> configurator)
{
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  s_FaplConfigurator = std::move(configurator);
}

FileIO FileIO::WriteFile(const std::filesystem::path& filepath)
{
  // Filesystem calls do not use HDF5. Keep them outside the HDF5 critical section.
  // A directory is never removed, empty or not, so a write to a directory path fails and leaves the directory intact. The status
  // follows links, so a link to a directory counts as a directory.
  std::error_code statusError;
  const std::filesystem::file_status status = std::filesystem::status(filepath, statusError);
  if(std::filesystem::is_directory(status))
  {
    std::cout << fmt::format("Cannot create file at path '{}'. The path is a directory.", PathToUtf8Text(filepath)) << std::endl;
    return {};
  }
  if(statusError && status.type() != std::filesystem::file_type::not_found)
  {
    std::cout << fmt::format("Cannot create file at path '{}'. Its status cannot be read: '{}'", PathToUtf8Text(filepath), statusError.message()) << std::endl;
    return {};
  }
  if(std::filesystem::exists(status))
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

  // The narrow name can throw on Windows, so it is built before the property list exists.
  const std::string pathText = filepath.string();
  hid_t fileId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t faplId = MakeFaplId();
    fileId = H5Fcreate(pathText.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, faplId);
    CloseFaplId(faplId);
  }
  if(fileId > 0)
  {
    return FileIO(filepath, fileId);
  }
  return {};
}

FileIO FileIO::AppendFile(const std::filesystem::path& filepath)
{
  // The narrow name can throw on Windows, so it is built before the property list exists. Keep the property-list lifecycle and file
  // open in one HDF5 critical section; the constructor does not call HDF5.
  const std::string pathText = filepath.string();
  hid_t fileId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t faplId = MakeFaplId();
    fileId = H5Fopen(pathText.c_str(), H5F_ACC_RDWR, faplId);
    CloseFaplId(faplId);
  }
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
  // Resolve the path before the non-recursive HDF5 lock.
  const std::string pathStr = getFilePath().string();
  hid_t id = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    id = H5Fopen(pathStr.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
  }
  setId(id);
  return id;
}

void FileIO::close()
{
  // Resolve the identifier before the non-recursive lock. This also serializes
  // destruction with other HDF5 calls.
  if(isOpen())
  {
    const hid_t selfId = getId();
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      H5Fclose(selfId);
    }
    setId(0);
  }
}

Result<> FileIO::closeChecked()
{
  // Read the stored identifier without getId(), which reopens a closed file, and before the non-recursive lock.
  if(!isOpen())
  {
    return {};
  }
  const hid_t selfId = getId();
  const std::string pathStr = PathToUtf8Text(getFilePath());

  // The proof and the close form one critical section, so no other thread can open a holder in between.
  std::string holderCause;
  herr_t closeStatus = 0;
  bool isIdStillValid = false;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    holderCause = DescribeOtherFileHolders(selfId);
    if(holderCause.empty())
    {
      closeStatus = H5Fclose(selfId);
      // When the close callback fails, HDF5 keeps the identifier registered, and only a later H5Fclose() releases it.
      isIdStillValid = closeStatus < 0 && H5Iis_valid(selfId) > 0;
    }
  }

  if(!holderCause.empty())
  {
    // The file stays open; the destructor closes it best-effort later.
    return MakeErrorResult(-6056, fmt::format("The HDF5 file '{}' was not closed because {}. Close every other handle to this file and write it again.", pathStr, holderCause));
  }

  if(closeStatus < 0)
  {
    if(!isIdStillValid)
    {
      setId(0);
    }
    // A still-valid identifier stays owned, so the destructor's best-effort close releases it.
    return MakeErrorResult(-6056, fmt::format("The HDF5 file '{}' failed to close, so its contents on disk may be incomplete. Check the free disk space and write it again.", pathStr));
  }
  setId(0);
  return {};
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

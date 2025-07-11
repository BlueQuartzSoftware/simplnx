#include "AtomicFile.hpp"

#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>
#include <fmt/std.h>

#include <iostream>
#include <random>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/xattr.h>
#endif

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
constexpr std::array<char, 62> k_Chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                                          'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
std::string CreateRandomDirName()
{
  std::mt19937_64 gen(static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::uniform_int_distribution<uint32> dist(0, k_Chars.size() - 1);

  std::string randomDir;
  for(uint32 i = 0; i < 24; i++)
  {
    randomDir += k_Chars[dist(gen)];
  }
  return randomDir;
}

bool SetExtendedAttribute(const fs::path& path, const std::string& key, const std::string& value)
{
#ifdef _WIN32
  // Set-Content -Path <path> -Stream <name> -Value <value>
  // Set-Content appends "/r/n" to the stream. Omitting this doesn't seem to affect the result
  // but we keep it here to stay as close as possible to the command example from Dropbox.
  // Opening an alternate data stream is done with a path of the form "<path>:<stream>"
  fs::path newPath = path;
  newPath += ":" + key;
  std::ofstream file(newPath);
  file << value << "\n";
  return file.good();
#elif defined(__linux__)
  // attr -s <name> -V <value> <path>
  // the `attr` command automatically prepends "user." to the key
  std::string newKey = "user." + key;
  return setxattr(path.c_str(), newKey.c_str(), value.data(), value.size(), 0) == 0;
#elif defined(__APPLE__)
  // xattr -w <name> <value> <path>
  return setxattr(path.c_str(), key.c_str(), value.data(), value.size(), 0, 0) == 0;
#endif
}

bool SetIgnoreFileSyncAttributes(const fs::path& path)
{
  // https://help.dropbox.com/sync/ignored-files
  bool result = SetExtendedAttribute(path, "com.dropbox.ignored", "1");

  // Dropbox notes that if you're using Dropbox for macOS on File Provider
  // you should use the following attribute instead. Since we don't have a good
  // way to detect that, we set both.
#ifdef __APPLE__
  result = result && SetExtendedAttribute(path, "com.apple.fileprovider.ignore#P", "1");
#endif

  return result;
}
} // namespace

Result<AtomicFile> AtomicFile::Create(fs::path filename)
{
  AtomicFile atomicFile(std::move(filename));

  // If the path is relative, then make it absolute
  if(!atomicFile.m_FilePath.is_absolute())
  {
    try
    {
      atomicFile.m_FilePath = fs::absolute(atomicFile.m_FilePath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult<AtomicFile>(-15780, fmt::format("AtomicFile Error: When attempting to create an absolute path, AtomicFile encountered the following error: '{}'", error.what()));
    }
  }

  // Validate write permissions
  {
    auto result = FileUtilities::ValidateDirectoryWritePermission(atomicFile.m_FilePath, true);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }
  }

  atomicFile.m_TempFilePath = fs::path(fmt::format("{}/{}/{}", atomicFile.m_FilePath.parent_path().string(), ::CreateRandomDirName(), atomicFile.m_FilePath.filename().string()));

  {
    auto parentPath = atomicFile.m_TempFilePath.parent_path();

    // Make sure any directory path is available as the user may have just typed
    // in a path without actually creating the full path
    auto result = CreateOutputDirectories(parentPath);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }
    // The temporary directory that AtomicFile creates should not be synced to cloud services
    // Since this is only necessary in synced folders if we fail we report and continue
    // rather than force a hard error.
    if(!SetIgnoreFileSyncAttributes(parentPath))
    {
      fmt::print("AtomicFile::Create: Unable to set ignore file sync attributes for '{}'", parentPath);
    }
  }

  return {std::move(atomicFile)};
}

AtomicFile::AtomicFile(fs::path filepath)
: m_FilePath(std::move(filepath))
{
}

AtomicFile::~AtomicFile() noexcept
{
  if(m_TempFilePath.empty())
  {
    return;
  }

  if(fs::exists(m_TempFilePath) || fs::exists(m_TempFilePath.parent_path()))
  {
    removeTempFile();
  }
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

Result<> AtomicFile::commit()
{
  if(!fs::exists(m_TempFilePath))
  {
    return MakeErrorResult(-15781, fmt::format("AtomicFile Commit Error: {} does not exist", m_TempFilePath.string()));
  }

  try
  {
    fs::rename(m_TempFilePath, m_FilePath);
  } catch(const std::filesystem::filesystem_error& error)
  {
    return MakeErrorResult(
        -15782, fmt::format("AtomicFile Commit Error: When attempting to move the temp file to the end absolute path, AtomicFile encountered the following error on rename(): '{}'", error.what()));
  }

  return {};
}

void AtomicFile::removeTempFile() const
{
  try
  {
    fs::remove_all(m_TempFilePath.parent_path());
  } catch(const std::exception& e)
  {
    std::cout << "AtomicFile::removeTempFile error: " << e.what() << std::endl;
  }
}

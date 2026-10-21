#include "AtomicFile.hpp"

#include "simplnx/Common/PathText.hpp"
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
    std::error_code absoluteError;
    fs::path absolutePath = fs::absolute(atomicFile.m_FilePath, absoluteError);
    if(absoluteError)
    {
      return MakeErrorResult<AtomicFile>(-15780, fmt::format("AtomicFile Error: When attempting to create an absolute path for '{}', AtomicFile encountered the following error: '{}'",
                                                             PathToUtf8Text(atomicFile.m_FilePath), absoluteError.message()));
    }
    atomicFile.m_FilePath = std::move(absolutePath);
  }

  // Validate write permissions
  {
    auto result = FileUtilities::ValidateDirectoryWritePermission(atomicFile.m_FilePath, true);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }
  }

  // Path composition keeps the native encoding. A narrow string() conversion would throw on Windows for a name that the active code page
  // cannot represent.
  atomicFile.m_TempFilePath = atomicFile.m_FilePath.parent_path() / ::CreateRandomDirName() / atomicFile.m_FilePath.filename();

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

  // removeTempFile() reports its own failures and an absent directory is not one, so no existence check (which can throw) is needed.
  removeTempFile();
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

Result<> AtomicFile::commit()
{
  // The error_code overloads report failures without exceptions, and PathToUtf8Text() renders any path without a narrow conversion.
  std::error_code statusError;
  if(!fs::exists(m_TempFilePath, statusError))
  {
    return MakeErrorResult(-15781, statusError ? fmt::format("AtomicFile Commit Error: cannot check whether '{}' exists: {}", PathToUtf8Text(m_TempFilePath), statusError.message()) :
                                                 fmt::format("AtomicFile Commit Error: {} does not exist", PathToUtf8Text(m_TempFilePath)));
  }

  std::error_code renameError;
  fs::rename(m_TempFilePath, m_FilePath, renameError);
  if(renameError)
  {
    return MakeErrorResult(-15782, fmt::format("AtomicFile Commit Error: When attempting to move the temp file '{}' to '{}', AtomicFile encountered the following error on rename(): '{}'",
                                               PathToUtf8Text(m_TempFilePath), PathToUtf8Text(m_FilePath), renameError.message()));
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

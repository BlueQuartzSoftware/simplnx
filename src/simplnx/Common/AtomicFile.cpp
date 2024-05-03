#include "AtomicFile.hpp"

#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <random>

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
      return MakeErrorResult<AtomicFile>(-15780, fmt::format("When attempting to create an absolute path, AtomicFile encountered the following error: '{}'", error.what()));
    }
  }

  // Validate write permissions
  { // Scope to avoid accessing result after it's no longer guaranteed by the move
    auto result = FileUtilities::ValidateDirectoryWritePermission(atomicFile.m_FilePath, true);
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
    }
  }

  atomicFile.m_TempFilePath = fs::path(fmt::format("{}/{}/{}", atomicFile.m_FilePath.parent_path().string(), ::CreateRandomDirName(), atomicFile.m_FilePath.filename().string()));
  { // Scope to avoid accessing result after it's no longer guaranteed by the move
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    auto result = CreateOutputDirectories(atomicFile.m_TempFilePath.parent_path());
    if(result.invalid())
    {
      return ConvertInvalidResult<AtomicFile>(std::move(result));
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
    return MakeErrorResult(-15780, m_TempFilePath.string() + " does not exist");
  }

  try
  {
    fs::rename(m_TempFilePath, m_FilePath);
  } catch(const std::filesystem::filesystem_error& error)
  {
    return MakeErrorResult(-15780, fmt::format("When attempting to move the temp file to the end absolute path, AtomicFile encountered the following error on rename(): '{}'", error.what()));
  }

  return {};
}

void AtomicFile::removeTempFile() const
{
  fs::remove_all(m_TempFilePath.parent_path());
}

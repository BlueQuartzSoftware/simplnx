#include "AtomicFile.hpp"

#include "simplnx/Utilities/FileUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <random>

using namespace nx::core;

namespace
{
constexpr std::array<char, 62> chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                                        'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
std::string randomDirName()
{
  std::mt19937_64 gen(static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::uniform_int_distribution<uint32> dist(0, chars.size() - 1);

  std::string randomDir = "";
  for(uint32 i = 0; i < 24; i++)
  {
    randomDir += chars[dist(gen)];
  }
  return randomDir;
}
} // namespace

AtomicFile::AtomicFile(const std::string& filename)
: m_FilePath(fs::path(filename))
, m_Result({})
{
  initialize();
}

AtomicFile::AtomicFile(fs::path&& filepath)
: m_FilePath(std::move(filepath))
, m_Result({})
{
  initialize();
}

AtomicFile::~AtomicFile()
{
  if(fs::exists(m_TempFilePath) || fs::exists(m_TempFilePath.parent_path()))
  {
    removeTempFile();
  }
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

bool AtomicFile::commit()
{
  if(!fs::exists(m_TempFilePath))
  {
    updateResult(MakeErrorResult(-15780, m_TempFilePath.string() + " does not exist"));
    return false;
  }

  try
  {
    fs::rename(m_TempFilePath, m_FilePath);
  } catch(const std::filesystem::filesystem_error& error)
  {
    updateResult(MakeErrorResult(-15780, fmt::format("When attempting to move the temp file to the end absolute path, AtomicFile encountered the following error on rename(): '{}'", error.what())));
    return false;
  }

  return true;
}

void AtomicFile::removeTempFile() const
{
  fs::remove_all(m_TempFilePath.parent_path());
}

Result<> AtomicFile::getResult() const
{
  return m_Result;
}

Result<> AtomicFile::createOutputDirectories()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  return CreateOutputDirectories(m_TempFilePath.parent_path());
}

void AtomicFile::clearResult()
{
  m_Result = {};
}

void AtomicFile::updateResult(Result<>&& result)
{
  m_Result = MergeResults(m_Result, result);
}

void AtomicFile::initialize()
{
  // If the path is relative, then make it absolute
  if(!m_FilePath.is_absolute())
  {
    try
    {
      m_FilePath = fs::absolute(m_FilePath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      updateResult(MakeErrorResult(-15780, fmt::format("When attempting to create an absolute path, AtomicFile encountered the following error: '{}'", error.what())));
    }
  }

  // Validate write permissions
  { // Scope to avoid accessing result after it's no longer guaranteed by the move
    auto result = FileUtilities::ValidateDirectoryWritePermission(m_FilePath, true);
    if(result.invalid())
    {
      updateResult(std::move(result));
    }
  }

  m_TempFilePath = fs::path(fmt::format("{}/{}/{}", m_FilePath.parent_path().string(), ::randomDirName(), m_FilePath.filename().string()));
  { // Scope to avoid accessing result after it's no longer guaranteed by the move
    auto result = createOutputDirectories();
    if(result.invalid())
    {
      updateResult(std::move(result));
    }
  }
}

#include "AtomicFile.hpp"

#include "complex/Utilities/FileUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

#include <random>

using namespace complex;

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

AtomicFile::AtomicFile(const std::string& filename, bool autoCommit)
: m_FilePath(fs::path(filename))
, m_AutoCommit(autoCommit)
, m_Result({})
{
  // If the path is relative, then make it absolute
  if(!m_FilePath.is_absolute())
  {
    try
    {
      m_FilePath = fs::absolute(m_FilePath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      m_Result = MergeResults(m_Result, MakeErrorResult(-15780, fmt::format("When attempting to create an absolute path, AtomicFile encountered the following error: '{}'", error.what())));
    }
  }

  // Validate write permissions
  auto result = FileUtilities::ValidateDirectoryWritePermission(m_FilePath, true);
  if(result.invalid())
  {
    m_Result = MergeResults(m_Result, result);
  }

  m_TempFilePath = fs::path(fmt::format("{}/{}/{}", m_FilePath.parent_path().string(), ::randomDirName(), m_FilePath.filename().string()));
  result = createOutputDirectories();
  if(result.invalid())
  {
    m_Result = MergeResults(m_Result, result);
  }
}

AtomicFile::AtomicFile(fs::path&& filepath, bool autoCommit)
: m_FilePath(std::move(filepath))
, m_AutoCommit(autoCommit)
, m_Result({})
{
  // If the path is relative, then make it absolute
  if(!m_FilePath.is_absolute())
  {
    try
    {
      m_FilePath = fs::absolute(m_FilePath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      m_Result = MergeResults(m_Result, MakeErrorResult(-15780, fmt::format("When attempting to create an absolute path, AtomicFile encountered the following error: '{}'", error.what())));
    }
  }

  // Validate write permissions
  auto result = FileUtilities::ValidateDirectoryWritePermission(m_FilePath, true);
  if(result.invalid())
  {
    m_Result = MergeResults(m_Result, result);
  }

  m_TempFilePath = fs::path(fmt::format("{}/{}/{}", m_FilePath.parent_path().string(), ::randomDirName(), m_FilePath.filename().string()));
  result = createOutputDirectories();
  if(result.invalid())
  {
    m_Result = MergeResults(m_Result, result);
  }
}

AtomicFile::~AtomicFile()
{
  if(m_AutoCommit)
  {
    commit();
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

void AtomicFile::commit() const
{
  if(!fs::exists(m_TempFilePath))
  {
    throw std::runtime_error(m_TempFilePath.string() + " does not exist");
  }

  fs::rename(m_TempFilePath, m_FilePath);
}

void AtomicFile::setAutoCommit(bool value)
{
  m_AutoCommit = value;
}

bool AtomicFile::getAutoCommit() const
{
  return m_AutoCommit;
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

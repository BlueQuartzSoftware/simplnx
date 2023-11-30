#include "AtomicFile.hpp"

#include "complex/Utilities/FilterUtilities.hpp"

#include <fmt/format.h>

using namespace complex;

AtomicFile::AtomicFile(const std::string& filename, bool autoCommit)
: m_FilePath(fs::path(filename))
, m_AutoCommit(autoCommit)
{
  m_TempFilePath = fs::path(fmt::format("{}/{}", fs::temp_directory_path().string(), m_FilePath.filename().string()));
}

AtomicFile::AtomicFile(fs::path&& filepath, bool autoCommit)
: m_FilePath(std::move(filepath))
, m_AutoCommit(autoCommit)
{
  m_TempFilePath = fs::path(fmt::format("{}/{}", fs::temp_directory_path().string(), m_FilePath.filename().string()));
}

AtomicFile::~AtomicFile()
{
  if(m_AutoCommit)
  {
    commit();
  }
  if(fs::exists(m_TempFilePath))
  {
    removeTempFile();
  }
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

void AtomicFile::commit()
{
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
  fs::remove(m_TempFilePath);
}

Result<> AtomicFile::createOutputDirectories()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  return CreateOutputDirectories(m_FilePath.parent_path());
}

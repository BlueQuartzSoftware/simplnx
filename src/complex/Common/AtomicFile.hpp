#pragma once

#include "complex/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace complex
{
namespace fs = std::filesystem;
class AtomicFile
{
public:
  explicit AtomicFile(const std::string& filename, bool autoCommit = false);
  explicit AtomicFile(fs::path&& filename, bool autoCommit = false);

  ~AtomicFile();

  fs::path tempFilePath() const;
  void commit();
  void setAutoCommit(bool value);
  bool getAutoCommit() const;
  void removeTempFile() const;
  Result<> createOutputDirectories();

private:
  fs::path m_FilePath;
  fs::path m_TempFilePath;
  bool m_AutoCommit = false;
};
} // namespace complex

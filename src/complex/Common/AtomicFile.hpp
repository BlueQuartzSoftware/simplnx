#pragma once

#include "complex/complex_export.hpp"

#include "complex/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace complex
{
namespace fs = std::filesystem;

/**
 * @class AtomicFile
 * @brief The AtomicFile class accepts a filepath which it stores and
 * used to create a temporary file. This temporary can be written to in place
 * of the original file path. Upon commit() the temporary file will be moved to
 * the end location passed in originally. By enabling autoCommit the temporary file
 * will be swapped upon object destruction. The temporary file will always be deleted
 * upon object destruction.
 */
class COMPLEX_EXPORT AtomicFile
{
public:
  explicit AtomicFile(const std::string& filename, bool autoCommit = false);
  explicit AtomicFile(fs::path&& filename, bool autoCommit = false);

  ~AtomicFile();

  fs::path tempFilePath() const;
  void commit() const;
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

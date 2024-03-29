#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace nx::core
{
namespace fs = std::filesystem;

/**
 * @class AtomicFile
 * @brief The AtomicFile class accepts a filepath which it stores and
 * used to create a temporary file. This temporary can be written to in place
 * of the original file path. Upon commit() the temporary file will be moved to
 * the end location passed in originally. The temporary file will always be deleted
 * upon object destruction.
 */
class SIMPLNX_EXPORT AtomicFile
{
public:
  /**
   * @brief Constructs the object and stores errors !!!CHECK getResult()!!!
   */
  explicit AtomicFile(const std::string& filename);
  /**
   * @brief Constructs the object and stores errors !!!CHECK getResult()!!!
   */
  explicit AtomicFile(fs::path&& filename);

  ~AtomicFile();

  /**
   * @brief Fetches the file path object to work from
   * @return fs::path the working file path modifications should be made to
   */
  [[nodiscard]] fs::path tempFilePath() const;

  /**
   * @brief Attempts to move the temp file to its final path !!!CHECK getResult() IF FALSE!!!
   * @return bool - if false you MUST call getResult() to error codes
   */
  [[nodiscard]] bool commit();

  /**
   * @brief Purges the temporary file
   */
  void removeTempFile() const;

  /**
   * @brief Fetch for amalgamated error codes !!!call clearResult() To Reset Errors/Warnings!!!
   * @return Result<> The result object containing all collected errors
   */
  [[nodiscard]] Result<> getResult() const;

  /**
   * @brief Empties the result container
   */
  void clearResult();

private:
  void updateResult(Result<>&& result);
  void initialize();
  fs::path m_FilePath;
  fs::path m_TempFilePath;
  Result<> m_Result;

  Result<> createOutputDirectories();
};
} // namespace nx::core

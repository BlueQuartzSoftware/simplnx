#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

// #include "highfive/H5File.hpp"
#include <H5Fpublic.h>

#include <filesystem>
#include <functional>
#include <string>

namespace nx::core::HDF5
{
/**
 * @class FileIO
 * @brief Owns one move-only HDF5 file identifier.
 *
 * Static open methods serialize their HDF5 calls with Support::ApiLock().
 * Different wrapper objects can operate on different threads, but concurrent
 * access to the same FileIO object is not supported. HDF5 receives a narrow
 * native path string.
 */
class SIMPLNX_EXPORT FileIO : public GroupIO
{
public:
  /**
   * @brief Opens an existing file read-only.
   * @param filepath Identifies the HDF5 file.
   * @return Valid wrapper on success, or an invalid wrapper on failure.
   *
   * Every attempt increments the process-wide read-open counter.
   */
  static FileIO ReadFile(const std::filesystem::path& filepath);

  /**
   * @brief Replaces an existing path and creates a new HDF5 file.
   * @param filepath Identifies the file to replace or create.
   * @return Valid wrapper on success, or an invalid wrapper on failure.
   *
   * Filesystem removal occurs before HDF5 creation and is not recoverable through this API.
   */
  static FileIO WriteFile(const std::filesystem::path& filepath);

  /**
   * @brief Opens an existing HDF5 file for read and write.
   * @param filepath Identifies the HDF5 file.
   * @return Valid wrapper on success, or an invalid wrapper on failure.
   */
  static FileIO AppendFile(const std::filesystem::path& filepath);

  /**
   * @brief Gets ReadFile() attempts since ResetReadOpenCount().
   * @return Atomic process-wide attempt count.
   *
   * Tests use this count to verify cache behavior without timing assertions.
   */
  static uint64 GetReadOpenCount();

  /**
   * @brief Resets the atomic ReadFile() attempt counter for test isolation.
   */
  static void ResetReadOpenCount();

  /**
   * @brief Sets the optional file-access property-list configurator.
   * @param configurator Receives a temporary HDF5 property-list ID. An empty callback clears the hook.
   *
   * Benchmarks use this test hook to install a latency-simulating HDF5 virtual
   * file driver. Production leaves it empty and uses H5P_DEFAULT.
   *
   * Assignment and invocation run under Support::ApiLock(), so they cannot race.
   * The callback also runs under this non-recursive lock. It must not call a
   * lock-taking HDF5 wrapper or SetFaplConfigurator().
   */
  static void SetFaplConfigurator(std::function<void(hid_t faplId)> configurator);

  FileIO() = default;

  FileIO(const FileIO& rhs) = delete;

  /**
   * @brief Moves file ownership.
   * @param rhs Supplies the file wrapper to move.
   */
  FileIO(FileIO&& rhs) noexcept = default;

  FileIO& operator=(const FileIO& rhs) = delete;
  FileIO& operator=(FileIO&& rhs) noexcept = default;

  /**
   * @brief Releases the HDF5 file ID.
   */
  ~FileIO() noexcept override;

  /**
   * @brief Gets the file name.
   * @return Final filesystem component, or an empty string for an invalid wrapper.
   */
  std::string getName() const override;

  /**
   * @brief Gets the root object's empty relative name path.
   * @return Empty string.
   */
  std::string getNamePath() const override;

  /**
   * @brief Gets the root object's empty HDF5 path.
   * @return Empty string.
   */
  std::string getObjectPath() const override;

#if 0
  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isGroup(const std::string& childName) const override;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isDataset(const std::string& childName) const override;
#endif

  /**
   * @brief Creates or opens an HDF5 dataset with the given name, dimensions, and datatype.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @param name
   * @param dims
   * @param dataType
   * @return HighFive::DataSet
   */
  // hid_t createOrOpenH5Dataset(const std::string& name, const DimsType& dims, DataType dataType) override;

protected:
  /**
   * @brief Takes ownership of one HDF5 file identifier.
   * @param filepath Stores the source path.
   * @param fileId Supplies the identifier to close during destruction.
   */
  FileIO(const std::filesystem::path& filepath, hid_t fileId);

  hid_t open() const override;
  void close() override;

private:
};
} // namespace nx::core::HDF5

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
   * @brief Replaces an existing file and creates a new HDF5 file.
   * @param filepath Identifies the file to replace or create.
   * @return Valid wrapper on success, or an invalid wrapper on failure, including when filepath (or the target of a link at
   * filepath) is a directory or its status cannot be read.
   *
   * Filesystem removal of an existing file occurs before HDF5 creation and is not recoverable through this API. A directory is
   * never removed.
   */
  static FileIO WriteFile(const std::filesystem::path& filepath);

  /**
   * @brief Opens an existing HDF5 file for read and write.
   * @param filepath Identifies the HDF5 file.
   * @return Valid wrapper on success, or an invalid wrapper on failure.
   *
   * Uses the same file-access property list as ReadFile() and WriteFile(), so a configurator installed by SetFaplConfigurator() also
   * reaches appends.
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
   * Tests use this hook to install an HDF5 virtual file driver (simulated latency,
   * a failing close) for ReadFile(), WriteFile() and AppendFile(). Production leaves
   * it empty and uses H5P_DEFAULT.
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
   * @brief Closes the file and reports whether the physical file really closed.
   * @return Success, or error -6056 naming the file path and the cause.
   *
   * With the default weak close degree, H5Fclose() succeeds while anything else keeps the physical file open and defers the real
   * close (and its final flush) to the last holder, so a writer would report success for a file that is not complete on disk. This
   * method first proves that this identifier is the file's only holder, then closes it. In one Support::ApiLock() scope it requires a
   * reference count of 1 on this identifier, no other file identifier for the same physical file, and no open dataset, group, named
   * datatype or attribute in the file through any identifier, and then calls H5Fclose().
   *
   * - A wrapper that holds no identifier succeeds without HDF5 calls.
   * - An extra holder, or a failed query, returns -6056 and leaves the file open; the destructor closes it best-effort later.
   * - A failed H5Fclose() returns -6056. When HDF5 keeps the identifier registered after the failure (a failing close callback
   *   does), this wrapper keeps owning it, and the destructor's best-effort close releases it. When the failed close already reached
   *   the file driver, that later close only frees HDF5's remaining records and does not close the driver again. An identifier that
   *   HDF5 released is cleared.
   * - Success releases the identifier, so isValid() returns false.
   *
   * close() and the destructor stay best-effort. Writers call this after a successful write, once every child handle of that write is
   * closed.
   * @pre The caller does not hold Support::ApiLock().
   */
  [[nodiscard]] Result<> closeChecked();

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

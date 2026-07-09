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
class SIMPLNX_EXPORT FileIO : public GroupIO
{
public:
  static FileIO ReadFile(const std::filesystem::path& filepath);
  static FileIO WriteFile(const std::filesystem::path& filepath);
  static FileIO AppendFile(const std::filesystem::path& filepath);

  /**
   * @brief Returns the number of times ReadFile() has opened a file since the
   * last ResetReadOpenCount(). Diagnostic used by tests to prove that cached
   * preflight paths perform zero file opens; asserting on operation counts is
   * deterministic where wall-clock timing is not.
   * @return uint64
   */
  static uint64 GetReadOpenCount();

  /**
   * @brief Resets the ReadFile() open counter. For test isolation.
   */
  static void ResetReadOpenCount();

  /**
   * @brief Installs a callback invoked with the file-access property list used
   * by ReadFile()/WriteFile() just before the file is opened.
   *
   * Why: benchmarks inject a latency-simulating HDF5 virtual file driver
   * through this hook to reproduce network-storage behavior deterministically
   * on local disks. Production code never sets it; when unset, files open with
   * the default property list when no configurator is installed.
   *
   * Threading contract: this is a test-only diagnostic hook. It stores the
   * callback without synchronization, so it must NOT be called while any
   * ReadFile()/WriteFile() may be running on another thread (ReadFile() is
   * invoked from preflight worker threads). Install the configurator before,
   * and clear it after, any concurrent I/O.
   * @param configurator Callback that receives the HDF5 file-access property
   * list id to configure (e.g. to select a virtual file driver). Passing an
   * empty function removes any previously installed configurator.
   */
  static void SetFaplConfigurator(std::function<void(hid_t faplId)> configurator);

  FileIO() = default;

  FileIO(const FileIO& rhs) = delete;

  /**
   * @brief Move constructor.
   * @param rhs
   */
  FileIO(FileIO&& rhs) noexcept = default;

  FileIO& operator=(const FileIO& rhs) = delete;
  FileIO& operator=(FileIO&& rhs) noexcept = default;

  /**
   * @brief Releases the HDF5 file ID.
   */
  ~FileIO() noexcept override;

  /**
   * @brief Returns the HDF5 file name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * @brief Overrides ObjectIO name path to return an empty string.
   * is invalid.
   * @return std::string
   */
  std::string getNamePath() const override;

  /**
   * Returns the HDF5 object path.
   * @return std::string
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
   * @brief Constructs a FileIO wrapping the HDF5 file at the target
   * filepath.
   * @param filepath
   * @param fileId
   */
  FileIO(const std::filesystem::path& filepath, hid_t fileId);

  hid_t open() const override;
  void close() override;

private:
};
} // namespace nx::core::HDF5

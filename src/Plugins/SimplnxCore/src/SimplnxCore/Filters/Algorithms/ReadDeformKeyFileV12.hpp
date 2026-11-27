#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
struct SIMPLNXCORE_EXPORT DataArrayMetadata
{
  DataPath path;
  usize tuple_count;
  usize component_count;
};

struct SIMPLNXCORE_EXPORT FileCache
{
  fs::path input_file;
  std::vector<DataArrayMetadata> data_arrays;
  usize vertex_attr_mat_tuple_count{};
  usize cell_attr_mat_tuple_count{};
  fs::file_time_type time_stamp;

  void flush()
  {
    input_file.clear();
    data_arrays.clear();
    time_stamp = fs::file_time_type();
  }
};

struct SIMPLNXCORE_EXPORT ReadDeformKeyFileV12InputValues
{
  fs::path input_file_path;
  DataPath quad_geom_path;
  DataPath vertex_am_path;
  DataPath cell_am_path;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ReadDeformKeyFileV12
{
public:
  ReadDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadDeformKeyFileV12InputValues* inputValues);
  ~ReadDeformKeyFileV12() noexcept;

  ReadDeformKeyFileV12(const ReadDeformKeyFileV12&) = delete;
  ReadDeformKeyFileV12(ReadDeformKeyFileV12&&) noexcept = delete;
  ReadDeformKeyFileV12& operator=(const ReadDeformKeyFileV12&) = delete;
  ReadDeformKeyFileV12& operator=(ReadDeformKeyFileV12&&) noexcept = delete;

  Result<> operator()(bool allocate);

  const std::atomic_bool& getCancel();
  void updateProgress(const std::string& progMessage);

  /**
   * @brief Reports a warning encountered while parsing. Access shim for the parser helper, which
   * cannot reach the message handler directly.
   * @param message
   */
  void reportWarning(const std::string& message);
  FileCache& getCache();

private:
  DataStructure& m_DataStructure;
  const ReadDeformKeyFileV12InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  FileCache m_Cache = {};
};
} // namespace nx::core
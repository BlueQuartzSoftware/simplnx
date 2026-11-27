#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{

struct NXIMAGEPROCESSING_EXPORT ReadVolumeGraphicsFileInputValues
{
  DataPath image_geometry_path;
  std::string cell_attribute_matrix_name;
  std::string density_array_name;
  fs::path vg_data_file;
};

class NXIMAGEPROCESSING_EXPORT ReadVolumeGraphicsFile
{
public:
  ReadVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadVolumeGraphicsFileInputValues* inputValues);
  ~ReadVolumeGraphicsFile() noexcept = default;

  ReadVolumeGraphicsFile(const ReadVolumeGraphicsFile&) = delete;
  ReadVolumeGraphicsFile(ReadVolumeGraphicsFile&&) noexcept = delete;
  ReadVolumeGraphicsFile& operator=(const ReadVolumeGraphicsFile&) = delete;
  ReadVolumeGraphicsFile& operator=(ReadVolumeGraphicsFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadVolumeGraphicsFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

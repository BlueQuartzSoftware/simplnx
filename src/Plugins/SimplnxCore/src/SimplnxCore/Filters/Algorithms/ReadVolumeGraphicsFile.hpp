#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadVolumeGraphicsFileInputValues
{
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string DensityArrayName;
  fs::path VGDataFile;
};

class SIMPLNXCORE_EXPORT ReadVolumeGraphicsFile
{
public:
  ReadVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadVolumeGraphicsFileInputValues* inputValues);
  ~ReadVolumeGraphicsFile() noexcept;

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

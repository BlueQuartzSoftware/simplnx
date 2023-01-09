#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{

struct COMPLEXCORE_EXPORT ImportVolumeGraphicsFileInputValues
{
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string DensityArrayName;
  fs::path VGDataFile;
};

class COMPLEXCORE_EXPORT ImportVolumeGraphicsFile
{
public:
  ImportVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ImportVolumeGraphicsFileInputValues* inputValues);
  ~ImportVolumeGraphicsFile() noexcept;

  ImportVolumeGraphicsFile(const ImportVolumeGraphicsFile&) = delete;
  ImportVolumeGraphicsFile(ImportVolumeGraphicsFile&&) noexcept = delete;
  ImportVolumeGraphicsFile& operator=(const ImportVolumeGraphicsFile&) = delete;
  ImportVolumeGraphicsFile& operator=(ImportVolumeGraphicsFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportVolumeGraphicsFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

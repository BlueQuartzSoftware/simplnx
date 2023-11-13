#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{

struct COMPLEXCORE_EXPORT ReadVolumeGraphicsFileInputValues
{
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string DensityArrayName;
  fs::path VGDataFile;
};

class COMPLEXCORE_EXPORT ReadVolumeGraphicsFile
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

} // namespace complex

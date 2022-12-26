#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ImportVolumeGraphicsFileInputValues
{
  FileSystemPathParameter::ValueType VGHeaderFile;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string DensityArrayName;
  std::filesystem::path VGDataFile;
};

class COMPLEXCORE_EXPORT ImportVolumeGraphicsFile
{
public:
  ImportVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportVolumeGraphicsFileInputValues* inputValues);
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

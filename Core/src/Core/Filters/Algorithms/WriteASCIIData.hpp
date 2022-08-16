#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct CORE_EXPORT WriteASCIIDataInputValues
{
  ChoicesParameter::ValueType outputStyle;
  FileSystemPathParameter::ValueType outputPath;
  StringParameter::ValueType fileExtension;
  int32 maxValPerLine;
  ChoicesParameter::ValueType delimiter;
  MultiArraySelectionParameter::ValueType selectedDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT WriteASCIIData
{
public:
  WriteASCIIData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteASCIIDataInputValues* inputValues);
  ~WriteASCIIData() noexcept;

  WriteASCIIData(const WriteASCIIData&) = delete;
  WriteASCIIData(WriteASCIIData&&) noexcept = delete;
  WriteASCIIData& operator=(const WriteASCIIData&) = delete;
  WriteASCIIData& operator=(WriteASCIIData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();
  void updateProgress(const std::string& progMessage);

private:
  DataStructure& m_DataStructure;
  const WriteASCIIDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  std::string getFilePath(const DataObject& selectedArrayPtr);
};

} // namespace complex

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

struct CORE_EXPORT WriteBinaryDataInputValues
{
  ChoicesParameter::ValueType endianess;
  FileSystemPathParameter::ValueType outputPath;
  StringParameter::ValueType fileExtension;
  MultiArraySelectionParameter::ValueType selectedDataArrayPaths;
};

/**
 * @class WriteBinaryData
 * @brief This filter will translate data from datastructures into binary and export it to a file with respect to endianess.
 */

class CORE_EXPORT WriteBinaryData
{
public:
  WriteBinaryData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteBinaryDataInputValues* inputValues);
  ~WriteBinaryData() noexcept;

  WriteBinaryData(const WriteBinaryData&) = delete;
  WriteBinaryData(WriteBinaryData&&) noexcept = delete;
  WriteBinaryData& operator=(const WriteBinaryData&) = delete;
  WriteBinaryData& operator=(WriteBinaryData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();
  void updateProgress(const std::string& progMessage);

private:
  DataStructure& m_DataStructure;
  const WriteBinaryDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  std::string getFilePath(const DataObject& selectedArrayPtr);
};

} // namespace complex

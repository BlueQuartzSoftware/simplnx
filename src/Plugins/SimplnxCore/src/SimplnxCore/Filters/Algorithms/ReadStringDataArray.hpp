#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{
namespace read_string_data_array
{
const ChoicesParameter::Choices k_Delimiters = {", (comma)", "; (semicolon)", "  (space)", ": (colon)", "\\t (Tab)", "New Line"};
const ChoicesParameter::ValueType k_CommaIndex = 0;
const ChoicesParameter::ValueType k_SemicolonIndex = 1;
const ChoicesParameter::ValueType k_SpaceIndex = 2;
const ChoicesParameter::ValueType k_ColonIndex = 3;
const ChoicesParameter::ValueType k_TabIndex = 4;
const ChoicesParameter::ValueType k_NewLineIndex = 5;
} // namespace read_string_data_array

struct SIMPLNXCORE_EXPORT ReadStringDataArrayInputValues
{
  FileSystemPathParameter::ValueType inputFileValue;
  usize skipLineCount;
  usize delimiterIndex;
  DataPath outputArrayPath;
  std::string dataFormat;
};

class SIMPLNXCORE_EXPORT ReadStringDataArray
{
public:
  ReadStringDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadStringDataArrayInputValues* inputValues);
  ~ReadStringDataArray() noexcept;

  ReadStringDataArray(const ReadStringDataArray&) = delete;
  ReadStringDataArray(ReadStringDataArray&&) noexcept = delete;
  ReadStringDataArray& operator=(const ReadStringDataArray&) = delete;
  ReadStringDataArray& operator=(ReadStringDataArray&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadStringDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

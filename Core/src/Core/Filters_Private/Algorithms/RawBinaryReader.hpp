#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RawBinaryReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.ScalarType = filterArgs.value<NumericType>(k_ScalarType_Key);
  inputValues.NumberOfComponents = filterArgs.value<int32>(k_NumberOfComponents_Key);
  inputValues.Endian = filterArgs.value<ChoicesParameter::ValueType>(k_Endian_Key);
  inputValues.SkipHeaderBytes = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  inputValues.CreatedAttributeArrayPath = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);

  return RawBinaryReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RawBinaryReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  NumericType ScalarType;
  int32 NumberOfComponents;
  ChoicesParameter::ValueType Endian;
  uint64 SkipHeaderBytes;
  DataPath CreatedAttributeArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RawBinaryReader
{
public:
  RawBinaryReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RawBinaryReaderInputValues* inputValues);
  ~RawBinaryReader() noexcept;

  RawBinaryReader(const RawBinaryReader&) = delete;
  RawBinaryReader(RawBinaryReader&&) noexcept = delete;
  RawBinaryReader& operator=(const RawBinaryReader&) = delete;
  RawBinaryReader& operator=(RawBinaryReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RawBinaryReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

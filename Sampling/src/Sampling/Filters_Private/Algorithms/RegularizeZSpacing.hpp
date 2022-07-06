#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RegularizeZSpacingInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.NewZRes = filterArgs.value<float32>(k_NewZRes_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  return RegularizeZSpacing(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT RegularizeZSpacingInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  float32 NewZRes;
  DataPath CellAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT RegularizeZSpacing
{
public:
  RegularizeZSpacing(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularizeZSpacingInputValues* inputValues);
  ~RegularizeZSpacing() noexcept;

  RegularizeZSpacing(const RegularizeZSpacing&) = delete;
  RegularizeZSpacing(RegularizeZSpacing&&) noexcept = delete;
  RegularizeZSpacing& operator=(const RegularizeZSpacing&) = delete;
  RegularizeZSpacing& operator=(RegularizeZSpacing&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RegularizeZSpacingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

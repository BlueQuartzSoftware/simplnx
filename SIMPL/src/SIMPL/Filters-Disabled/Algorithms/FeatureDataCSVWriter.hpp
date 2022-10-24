#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FeatureDataCSVWriterInputValues inputValues;

  inputValues.FeatureDataFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_FeatureDataFile_Key);
  inputValues.WriteNeighborListData = filterArgs.value<bool>(k_WriteNeighborListData_Key);
  inputValues.WriteNumFeaturesLine = filterArgs.value<bool>(k_WriteNumFeaturesLine_Key);
  inputValues.DelimiterChoiceInt = filterArgs.value<ChoicesParameter::ValueType>(k_DelimiterChoiceInt_Key);
  inputValues.CellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);

  return FeatureDataCSVWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT FeatureDataCSVWriterInputValues
{
  FileSystemPathParameter::ValueType FeatureDataFile;
  bool WriteNeighborListData;
  bool WriteNumFeaturesLine;
  ChoicesParameter::ValueType DelimiterChoiceInt;
  DataPath CellFeatureAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT FeatureDataCSVWriter
{
public:
  FeatureDataCSVWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureDataCSVWriterInputValues* inputValues);
  ~FeatureDataCSVWriter() noexcept;

  FeatureDataCSVWriter(const FeatureDataCSVWriter&) = delete;
  FeatureDataCSVWriter(FeatureDataCSVWriter&&) noexcept = delete;
  FeatureDataCSVWriter& operator=(const FeatureDataCSVWriter&) = delete;
  FeatureDataCSVWriter& operator=(FeatureDataCSVWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FeatureDataCSVWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

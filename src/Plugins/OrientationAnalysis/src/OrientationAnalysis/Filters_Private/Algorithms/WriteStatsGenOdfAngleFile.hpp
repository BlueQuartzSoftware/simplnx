#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WriteStatsGenOdfAngleFileInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.Weight = filterArgs.value<float32>(k_Weight_Key);
  inputValues.Sigma = filterArgs.value<int32>(k_Sigma_Key);
  inputValues.Delimiter = filterArgs.value<ChoicesParameter::ValueType>(k_Delimiter_Key);
  inputValues.ConvertToDegrees = filterArgs.value<bool>(k_ConvertToDegrees_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  return WriteStatsGenOdfAngleFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  float32 Weight;
  int32 Sigma;
  ChoicesParameter::ValueType Delimiter;
  bool ConvertToDegrees;
  bool UseGoodVoxels;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFile
{
public:
  WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStatsGenOdfAngleFileInputValues* inputValues);
  ~WriteStatsGenOdfAngleFile() noexcept;

  WriteStatsGenOdfAngleFile(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile(WriteStatsGenOdfAngleFile&&) noexcept = delete;
  WriteStatsGenOdfAngleFile& operator=(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile& operator=(WriteStatsGenOdfAngleFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteStatsGenOdfAngleFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

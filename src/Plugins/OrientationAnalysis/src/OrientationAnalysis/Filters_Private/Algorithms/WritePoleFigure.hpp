#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WritePoleFigureInputValues inputValues;

  inputValues.Title = filterArgs.value<StringParameter::ValueType>(k_Title_Key);
  inputValues.GenerationAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  inputValues.LambertSize = filterArgs.value<int32>(k_LambertSize_Key);
  inputValues.NumColors = filterArgs.value<int32>(k_NumColors_Key);
  inputValues.ImageFormat = filterArgs.value<ChoicesParameter::ValueType>(k_ImageFormat_Key);
  inputValues.ImageLayout = filterArgs.value<ChoicesParameter::ValueType>(k_ImageLayout_Key);
  inputValues.OutputPath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.ImagePrefix = filterArgs.value<StringParameter::ValueType>(k_ImagePrefix_Key);
  inputValues.ImageSize = filterArgs.value<int32>(k_ImageSize_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.MaterialNameArrayPath = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);

  return WritePoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT WritePoleFigureInputValues
{
  StringParameter::ValueType Title;
  ChoicesParameter::ValueType GenerationAlgorithm;
  int32 LambertSize;
  int32 NumColors;
  ChoicesParameter::ValueType ImageFormat;
  ChoicesParameter::ValueType ImageLayout;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType ImagePrefix;
  int32 ImageSize;
  bool UseGoodVoxels;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MaterialNameArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT WritePoleFigure
{
public:
  WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues);
  ~WritePoleFigure() noexcept;

  WritePoleFigure(const WritePoleFigure&) = delete;
  WritePoleFigure(WritePoleFigure&&) noexcept = delete;
  WritePoleFigure& operator=(const WritePoleFigure&) = delete;
  WritePoleFigure& operator=(WritePoleFigure&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WritePoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

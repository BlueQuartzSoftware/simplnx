#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InitializeDataInputValues inputValues;

  inputValues.CellAttributeMatrixPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CellAttributeMatrixPaths_Key);
  inputValues.XMin = filterArgs.value<int32>(k_XMin_Key);
  inputValues.YMin = filterArgs.value<int32>(k_YMin_Key);
  inputValues.ZMin = filterArgs.value<int32>(k_ZMin_Key);
  inputValues.XMax = filterArgs.value<int32>(k_XMax_Key);
  inputValues.YMax = filterArgs.value<int32>(k_YMax_Key);
  inputValues.ZMax = filterArgs.value<int32>(k_ZMax_Key);
  inputValues.InitType = filterArgs.value<ChoicesParameter::ValueType>(k_InitType_Key);
  inputValues.InitValue = filterArgs.value<float64>(k_InitValue_Key);
  inputValues.InitRange = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InitRange_Key);

  return InitializeData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT InitializeDataInputValues
{
  MultiArraySelectionParameter::ValueType CellAttributeMatrixPaths;
  int32 XMin;
  int32 YMin;
  int32 ZMin;
  int32 XMax;
  int32 YMax;
  int32 ZMax;
  ChoicesParameter::ValueType InitType;
  float64 InitValue;
  <<<NOT_IMPLEMENTED>>> InitRange;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT InitializeData
{
public:
  InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues);
  ~InitializeData() noexcept;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;
  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InitializeDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

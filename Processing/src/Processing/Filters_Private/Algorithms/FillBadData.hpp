#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FillBadDataInputValues inputValues;

  inputValues.MinAllowedDefectSize = filterArgs.value<int32>(k_MinAllowedDefectSize_Key);
  inputValues.StoreAsNewPhase = filterArgs.value<bool>(k_StoreAsNewPhase_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return FillBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT FillBadDataInputValues
{
  int32 MinAllowedDefectSize;
  bool StoreAsNewPhase;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT FillBadData
{
public:
  FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues);
  ~FillBadData() noexcept;

  FillBadData(const FillBadData&) = delete;
  FillBadData(FillBadData&&) noexcept = delete;
  FillBadData& operator=(const FillBadData&) = delete;
  FillBadData& operator=(FillBadData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FillBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

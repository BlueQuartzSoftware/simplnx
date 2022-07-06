#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindSizesInputValues inputValues;

  inputValues.SaveElementSizes = filterArgs.value<bool>(k_SaveElementSizes_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.EquivalentDiametersArrayName = filterArgs.value<DataPath>(k_EquivalentDiametersArrayName_Key);
  inputValues.NumElementsArrayName = filterArgs.value<DataPath>(k_NumElementsArrayName_Key);
  inputValues.VolumesArrayName = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  return FindSizes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct STATSTOOLBOX_EXPORT FindSizesInputValues
{
  bool SaveElementSizes;
  DataPath FeatureIdsArrayPath;
  DataPath FeatureAttributeMatrixName;
  DataPath EquivalentDiametersArrayName;
  DataPath NumElementsArrayName;
  DataPath VolumesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class STATSTOOLBOX_EXPORT FindSizes
{
public:
  FindSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSizesInputValues* inputValues);
  ~FindSizes() noexcept;

  FindSizes(const FindSizes&) = delete;
  FindSizes(FindSizes&&) noexcept = delete;
  FindSizes& operator=(const FindSizes&) = delete;
  FindSizes& operator=(FindSizes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

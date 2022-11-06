#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractFlaggedFeaturesInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FlaggedFeaturesArrayPath = filterArgs.value<DataPath>(k_FlaggedFeaturesArrayPath_Key);

  return ExtractFlaggedFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT ExtractFlaggedFeaturesInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FlaggedFeaturesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT ExtractFlaggedFeatures
{
public:
  ExtractFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractFlaggedFeaturesInputValues* inputValues);
  ~ExtractFlaggedFeatures() noexcept;

  ExtractFlaggedFeatures(const ExtractFlaggedFeatures&) = delete;
  ExtractFlaggedFeatures(ExtractFlaggedFeatures&&) noexcept = delete;
  ExtractFlaggedFeatures& operator=(const ExtractFlaggedFeatures&) = delete;
  ExtractFlaggedFeatures& operator=(ExtractFlaggedFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

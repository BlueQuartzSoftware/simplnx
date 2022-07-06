#pragma once

#include "Generic/Generic_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindBoundingBoxFeaturesInputValues inputValues;

  inputValues.CalcByPhase = filterArgs.value<bool>(k_CalcByPhase_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.SurfaceFeaturesArrayPath = filterArgs.value<DataPath>(k_SurfaceFeaturesArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  inputValues.BiasedFeaturesArrayName = filterArgs.value<DataPath>(k_BiasedFeaturesArrayName_Key);

  return FindBoundingBoxFeatures(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct GENERIC_EXPORT FindBoundingBoxFeaturesInputValues
{
  bool CalcByPhase;
  DataPath CentroidsArrayPath;
  DataPath SurfaceFeaturesArrayPath;
  DataPath PhasesArrayPath;
  DataPath BiasedFeaturesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class GENERIC_EXPORT FindBoundingBoxFeatures
{
public:
  FindBoundingBoxFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundingBoxFeaturesInputValues* inputValues);
  ~FindBoundingBoxFeatures() noexcept;

  FindBoundingBoxFeatures(const FindBoundingBoxFeatures&) = delete;
  FindBoundingBoxFeatures(FindBoundingBoxFeatures&&) noexcept = delete;
  FindBoundingBoxFeatures& operator=(const FindBoundingBoxFeatures&) = delete;
  FindBoundingBoxFeatures& operator=(FindBoundingBoxFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundingBoxFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

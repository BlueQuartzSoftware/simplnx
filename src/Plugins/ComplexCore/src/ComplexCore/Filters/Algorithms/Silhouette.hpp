#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  SilhouetteInputValues inputValues;

  inputValues.DistanceMetric = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.SilhouetteArrayPath = filterArgs.value<DataPath>(k_SilhouetteArrayPath_Key);

  return Silhouette(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT SilhouetteInputValues
{
  ChoicesParameter::ValueType DistanceMetric;
  bool UseMask;
  DataPath SelectedArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath SilhouetteArrayPath;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT Silhouette
{
public:
  Silhouette(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SilhouetteInputValues* inputValues);
  ~Silhouette() noexcept;

  Silhouette(const Silhouette&) = delete;
  Silhouette(Silhouette&&) noexcept = delete;
  Silhouette& operator=(const Silhouette&) = delete;
  Silhouette& operator=(Silhouette&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SilhouetteInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

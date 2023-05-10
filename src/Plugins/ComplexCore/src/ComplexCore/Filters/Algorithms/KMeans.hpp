#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  KMeansInputValues inputValues;

  inputValues.InitClusters = filterArgs.value<int32>(k_InitClusters_Key);
  inputValues.DistanceMetric = filterArgs.value<ChoicesParameter::ValueType>(k_DistanceMetric_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.MeansArrayName = filterArgs.value<DataPath>(k_MeansArrayName_Key);

  return KMeans(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT KMeansInputValues
{
  int32 InitClusters;
  ChoicesParameter::ValueType DistanceMetric;
  bool UseMask;
  DataPath SelectedArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayName;
  DataPath FeatureAttributeMatrixName;
  DataPath MeansArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT KMeans
{
public:
  KMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KMeansInputValues* inputValues);
  ~KMeans() noexcept;

  KMeans(const KMeans&) = delete;
  KMeans(KMeans&&) noexcept = delete;
  KMeans& operator=(const KMeans&) = delete;
  KMeans& operator=(KMeans&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

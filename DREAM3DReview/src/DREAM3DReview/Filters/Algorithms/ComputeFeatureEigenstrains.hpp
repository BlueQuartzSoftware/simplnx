#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeFeatureEigenstrainsInputValues inputValues;

  inputValues.PoissonRatio = filterArgs.value<float32>(k_PoissonRatio_Key);
  inputValues.UseEllipsoidalGrains = filterArgs.value<bool>(k_UseEllipsoidalGrains_Key);
  inputValues.UseCorrectionalMatrix = filterArgs.value<bool>(k_UseCorrectionalMatrix_Key);
  inputValues.Beta11 = filterArgs.value<float32>(k_Beta11_Key);
  inputValues.Beta22 = filterArgs.value<float32>(k_Beta22_Key);
  inputValues.Beta33 = filterArgs.value<float32>(k_Beta33_Key);
  inputValues.Beta23 = filterArgs.value<float32>(k_Beta23_Key);
  inputValues.Beta13 = filterArgs.value<float32>(k_Beta13_Key);
  inputValues.Beta12 = filterArgs.value<float32>(k_Beta12_Key);
  inputValues.AxisLengthsArrayPath = filterArgs.value<DataPath>(k_AxisLengthsArrayPath_Key);
  inputValues.AxisEulerAnglesArrayPath = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayPath_Key);
  inputValues.ElasticStrainsArrayPath = filterArgs.value<DataPath>(k_ElasticStrainsArrayPath_Key);
  inputValues.EigenstrainsArrayName = filterArgs.value<DataPath>(k_EigenstrainsArrayName_Key);

  return ComputeFeatureEigenstrains(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ComputeFeatureEigenstrainsInputValues
{
  float32 PoissonRatio;
  bool UseEllipsoidalGrains;
  bool UseCorrectionalMatrix;
  float32 Beta11;
  float32 Beta22;
  float32 Beta33;
  float32 Beta23;
  float32 Beta13;
  float32 Beta12;
  DataPath AxisLengthsArrayPath;
  DataPath AxisEulerAnglesArrayPath;
  DataPath ElasticStrainsArrayPath;
  DataPath EigenstrainsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ComputeFeatureEigenstrains
{
public:
  ComputeFeatureEigenstrains(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureEigenstrainsInputValues* inputValues);
  ~ComputeFeatureEigenstrains() noexcept;

  ComputeFeatureEigenstrains(const ComputeFeatureEigenstrains&) = delete;
  ComputeFeatureEigenstrains(ComputeFeatureEigenstrains&&) noexcept = delete;
  ComputeFeatureEigenstrains& operator=(const ComputeFeatureEigenstrains&) = delete;
  ComputeFeatureEigenstrains& operator=(ComputeFeatureEigenstrains&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureEigenstrainsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

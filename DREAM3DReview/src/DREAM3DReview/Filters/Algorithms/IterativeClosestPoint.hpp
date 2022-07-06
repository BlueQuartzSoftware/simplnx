#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  IterativeClosestPointInputValues inputValues;

  inputValues.MovingVertexGeometry = filterArgs.value<DataPath>(k_MovingVertexGeometry_Key);
  inputValues.TargetVertexGeometry = filterArgs.value<DataPath>(k_TargetVertexGeometry_Key);
  inputValues.Iterations = filterArgs.value<int32>(k_Iterations_Key);
  inputValues.ApplyTransform = filterArgs.value<bool>(k_ApplyTransform_Key);
  inputValues.TransformAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_TransformAttributeMatrixName_Key);
  inputValues.TransformArrayName = filterArgs.value<StringParameter::ValueType>(k_TransformArrayName_Key);

  return IterativeClosestPoint(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT IterativeClosestPointInputValues
{
  DataPath MovingVertexGeometry;
  DataPath TargetVertexGeometry;
  int32 Iterations;
  bool ApplyTransform;
  StringParameter::ValueType TransformAttributeMatrixName;
  StringParameter::ValueType TransformArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT IterativeClosestPoint
{
public:
  IterativeClosestPoint(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IterativeClosestPointInputValues* inputValues);
  ~IterativeClosestPoint() noexcept;

  IterativeClosestPoint(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint(IterativeClosestPoint&&) noexcept = delete;
  IterativeClosestPoint& operator=(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint& operator=(IterativeClosestPoint&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IterativeClosestPointInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

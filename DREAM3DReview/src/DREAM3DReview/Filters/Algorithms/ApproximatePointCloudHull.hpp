#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ApproximatePointCloudHullInputValues inputValues;

  inputValues.GridResolution = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  inputValues.NumberOfEmptyNeighbors = filterArgs.value<int32>(k_NumberOfEmptyNeighbors_Key);
  inputValues.VertexDataContainerName = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  inputValues.HullDataContainerName = filterArgs.value<StringParameter::ValueType>(k_HullDataContainerName_Key);

  return ApproximatePointCloudHull(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ApproximatePointCloudHullInputValues
{
  VectorFloat32Parameter::ValueType GridResolution;
  int32 NumberOfEmptyNeighbors;
  DataPath VertexDataContainerName;
  StringParameter::ValueType HullDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ApproximatePointCloudHull
{
public:
  ApproximatePointCloudHull(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ApproximatePointCloudHullInputValues* inputValues);
  ~ApproximatePointCloudHull() noexcept;

  ApproximatePointCloudHull(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull(ApproximatePointCloudHull&&) noexcept = delete;
  ApproximatePointCloudHull& operator=(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull& operator=(ApproximatePointCloudHull&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ApproximatePointCloudHullInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DownsampleVertexGeometryInputValues inputValues;

  inputValues.DownsampleType = filterArgs.value<ChoicesParameter::ValueType>(k_DownsampleType_Key);
  inputValues.DecimationFreq = filterArgs.value<int32>(k_DecimationFreq_Key);
  inputValues.DecimationFraction = filterArgs.value<float32>(k_DecimationFraction_Key);
  inputValues.GridResolution = filterArgs.value<VectorFloat32Parameter::ValueType>(k_GridResolution_Key);
  inputValues.VertexAttrMatPath = filterArgs.value<DataPath>(k_VertexAttrMatPath_Key);

  return DownsampleVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT DownsampleVertexGeometryInputValues
{
  ChoicesParameter::ValueType DownsampleType;
  int32 DecimationFreq;
  float32 DecimationFraction;
  VectorFloat32Parameter::ValueType GridResolution;
  DataPath VertexAttrMatPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT DownsampleVertexGeometry
{
public:
  DownsampleVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DownsampleVertexGeometryInputValues* inputValues);
  ~DownsampleVertexGeometry() noexcept;

  DownsampleVertexGeometry(const DownsampleVertexGeometry&) = delete;
  DownsampleVertexGeometry(DownsampleVertexGeometry&&) noexcept = delete;
  DownsampleVertexGeometry& operator=(const DownsampleVertexGeometry&) = delete;
  DownsampleVertexGeometry& operator=(DownsampleVertexGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DownsampleVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

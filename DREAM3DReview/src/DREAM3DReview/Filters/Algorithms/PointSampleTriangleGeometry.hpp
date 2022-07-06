#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PointSampleTriangleGeometryInputValues inputValues;

  inputValues.SamplesNumberType = filterArgs.value<ChoicesParameter::ValueType>(k_SamplesNumberType_Key);
  inputValues.NumberOfSamples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.TriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  inputValues.ParentGeometry = filterArgs.value<DataPath>(k_ParentGeometry_Key);
  inputValues.TriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.VertexGeometry = filterArgs.value<StringParameter::ValueType>(k_VertexGeometry_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);

  return PointSampleTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT PointSampleTriangleGeometryInputValues
{
  ChoicesParameter::ValueType SamplesNumberType;
  int32 NumberOfSamples;
  bool UseMask;
  DataPath TriangleGeometry;
  DataPath ParentGeometry;
  DataPath TriangleAreasArrayPath;
  DataPath MaskArrayPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  StringParameter::ValueType VertexGeometry;
  DataPath VertexAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT PointSampleTriangleGeometry
{
public:
  PointSampleTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PointSampleTriangleGeometryInputValues* inputValues);
  ~PointSampleTriangleGeometry() noexcept;

  PointSampleTriangleGeometry(const PointSampleTriangleGeometry&) = delete;
  PointSampleTriangleGeometry(PointSampleTriangleGeometry&&) noexcept = delete;
  PointSampleTriangleGeometry& operator=(const PointSampleTriangleGeometry&) = delete;
  PointSampleTriangleGeometry& operator=(PointSampleTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PointSampleTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

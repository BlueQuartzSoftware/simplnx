#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTriangleGeomShapesInputValues inputValues;

  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.VolumesArrayPath = filterArgs.value<DataPath>(k_VolumesArrayPath_Key);
  inputValues.Omega3sArrayName = filterArgs.value<DataPath>(k_Omega3sArrayName_Key);
  inputValues.AxisLengthsArrayName = filterArgs.value<DataPath>(k_AxisLengthsArrayName_Key);
  inputValues.AxisEulerAnglesArrayName = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayName_Key);
  inputValues.AspectRatiosArrayName = filterArgs.value<DataPath>(k_AspectRatiosArrayName_Key);

  return FindTriangleGeomShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT FindTriangleGeomShapesInputValues
{
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixName;
  DataPath CentroidsArrayPath;
  DataPath VolumesArrayPath;
  DataPath Omega3sArrayName;
  DataPath AxisLengthsArrayName;
  DataPath AxisEulerAnglesArrayName;
  DataPath AspectRatiosArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT FindTriangleGeomShapes
{
public:
  FindTriangleGeomShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomShapesInputValues* inputValues);
  ~FindTriangleGeomShapes() noexcept;

  FindTriangleGeomShapes(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes(FindTriangleGeomShapes&&) noexcept = delete;
  FindTriangleGeomShapes& operator=(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes& operator=(FindTriangleGeomShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

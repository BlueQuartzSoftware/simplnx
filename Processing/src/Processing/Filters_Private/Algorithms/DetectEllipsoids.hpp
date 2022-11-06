#pragma once

#include "Processing/Processing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  DetectEllipsoidsInputValues inputValues;

  inputValues.MinFiberAxisLength = filterArgs.value<int32>(k_MinFiberAxisLength_Key);
  inputValues.MaxFiberAxisLength = filterArgs.value<int32>(k_MaxFiberAxisLength_Key);
  inputValues.HoughTransformThreshold = filterArgs.value<float32>(k_HoughTransformThreshold_Key);
  inputValues.MinAspectRatio = filterArgs.value<float32>(k_MinAspectRatio_Key);
  inputValues.ImageScaleBarLength = filterArgs.value<int32>(k_ImageScaleBarLength_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  inputValues.EllipseFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_EllipseFeatureAttributeMatrixPath_Key);
  inputValues.CenterCoordinatesArrayName = filterArgs.value<DataPath>(k_CenterCoordinatesArrayName_Key);
  inputValues.MajorAxisLengthArrayName = filterArgs.value<DataPath>(k_MajorAxisLengthArrayName_Key);
  inputValues.MinorAxisLengthArrayName = filterArgs.value<DataPath>(k_MinorAxisLengthArrayName_Key);
  inputValues.RotationalAnglesArrayName = filterArgs.value<DataPath>(k_RotationalAnglesArrayName_Key);
  inputValues.DetectedEllipsoidsFeatureIdsArrayPath = filterArgs.value<DataPath>(k_DetectedEllipsoidsFeatureIdsArrayPath_Key);

  return DetectEllipsoids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROCESSING_EXPORT DetectEllipsoidsInputValues
{
  int32 MinFiberAxisLength;
  int32 MaxFiberAxisLength;
  float32 HoughTransformThreshold;
  float32 MinAspectRatio;
  int32 ImageScaleBarLength;
  DataPath FeatureIdsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath EllipseFeatureAttributeMatrixPath;
  DataPath CenterCoordinatesArrayName;
  DataPath MajorAxisLengthArrayName;
  DataPath MinorAxisLengthArrayName;
  DataPath RotationalAnglesArrayName;
  DataPath DetectedEllipsoidsFeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROCESSING_EXPORT DetectEllipsoids
{
public:
  DetectEllipsoids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DetectEllipsoidsInputValues* inputValues);
  ~DetectEllipsoids() noexcept;

  DetectEllipsoids(const DetectEllipsoids&) = delete;
  DetectEllipsoids(DetectEllipsoids&&) noexcept = delete;
  DetectEllipsoids& operator=(const DetectEllipsoids&) = delete;
  DetectEllipsoids& operator=(DetectEllipsoids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const DetectEllipsoidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

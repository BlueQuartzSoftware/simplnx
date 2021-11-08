#include "DetectEllipsoids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string DetectEllipsoids::name() const
{
  return FilterTraits<DetectEllipsoids>::name.str();
}

//------------------------------------------------------------------------------
std::string DetectEllipsoids::className() const
{
  return FilterTraits<DetectEllipsoids>::className;
}

//------------------------------------------------------------------------------
Uuid DetectEllipsoids::uuid() const
{
  return FilterTraits<DetectEllipsoids>::uuid;
}

//------------------------------------------------------------------------------
std::string DetectEllipsoids::humanName() const
{
  return "Detect 2D Ellipses";
}

//------------------------------------------------------------------------------
std::vector<std::string> DetectEllipsoids::defaultTags() const
{
  return {"#Processing", "#Generation"};
}

//------------------------------------------------------------------------------
Parameters DetectEllipsoids::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_MinFiberAxisLength_Key, "Min Fiber Axis Length", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_MaxFiberAxisLength_Key, "Max Fiber Axis Length", "", 1234356));
  params.insert(std::make_unique<Float32Parameter>(k_HoughTransformThreshold_Key, "Threshold for Hough Transform", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_MinAspectRatio_Key, "Minimum Aspect Ratio", "", 1.23345f));
  params.insert(std::make_unique<Int32Parameter>(k_ImageScaleBarLength_Key, "Length of Image Scale Bar", "", 1234356));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_EllipseFeatureAttributeMatrixPath_Key, "Ellipsoid Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CenterCoordinatesArrayName_Key, "Ellipsoid Center Coordinates", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MajorAxisLengthArrayName_Key, "Ellipsoid Major Axis Lengths", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MinorAxisLengthArrayName_Key, "Ellipsoid Minor Axis Lengths", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_RotationalAnglesArrayName_Key, "Ellipsoid Rotational Angles", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DetectedEllipsoidsFeatureIdsArrayPath_Key, "Detected Ellipsoids Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DetectEllipsoids::clone() const
{
  return std::make_unique<DetectEllipsoids>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DetectEllipsoids::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMinFiberAxisLengthValue = filterArgs.value<int32>(k_MinFiberAxisLength_Key);
  auto pMaxFiberAxisLengthValue = filterArgs.value<int32>(k_MaxFiberAxisLength_Key);
  auto pHoughTransformThresholdValue = filterArgs.value<float32>(k_HoughTransformThreshold_Key);
  auto pMinAspectRatioValue = filterArgs.value<float32>(k_MinAspectRatio_Key);
  auto pImageScaleBarLengthValue = filterArgs.value<int32>(k_ImageScaleBarLength_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pEllipseFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_EllipseFeatureAttributeMatrixPath_Key);
  auto pCenterCoordinatesArrayNameValue = filterArgs.value<DataPath>(k_CenterCoordinatesArrayName_Key);
  auto pMajorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MajorAxisLengthArrayName_Key);
  auto pMinorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MinorAxisLengthArrayName_Key);
  auto pRotationalAnglesArrayNameValue = filterArgs.value<DataPath>(k_RotationalAnglesArrayName_Key);
  auto pDetectedEllipsoidsFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_DetectedEllipsoidsFeatureIdsArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<DetectEllipsoidsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> DetectEllipsoids::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinFiberAxisLengthValue = filterArgs.value<int32>(k_MinFiberAxisLength_Key);
  auto pMaxFiberAxisLengthValue = filterArgs.value<int32>(k_MaxFiberAxisLength_Key);
  auto pHoughTransformThresholdValue = filterArgs.value<float32>(k_HoughTransformThreshold_Key);
  auto pMinAspectRatioValue = filterArgs.value<float32>(k_MinAspectRatio_Key);
  auto pImageScaleBarLengthValue = filterArgs.value<int32>(k_ImageScaleBarLength_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pEllipseFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_EllipseFeatureAttributeMatrixPath_Key);
  auto pCenterCoordinatesArrayNameValue = filterArgs.value<DataPath>(k_CenterCoordinatesArrayName_Key);
  auto pMajorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MajorAxisLengthArrayName_Key);
  auto pMinorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MinorAxisLengthArrayName_Key);
  auto pRotationalAnglesArrayNameValue = filterArgs.value<DataPath>(k_RotationalAnglesArrayName_Key);
  auto pDetectedEllipsoidsFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_DetectedEllipsoidsFeatureIdsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

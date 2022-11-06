#include "DetectEllipsoids.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
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
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
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
IFilter::PreflightResult DetectEllipsoids::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMinFiberAxisLengthValue = filterArgs.value<int32>(k_MinFiberAxisLength_Key);
  auto pMaxFiberAxisLengthValue = filterArgs.value<int32>(k_MaxFiberAxisLength_Key);
  auto pHoughTransformThresholdValue = filterArgs.value<float32>(k_HoughTransformThreshold_Key);
  auto pMinAspectRatioValue = filterArgs.value<float32>(k_MinAspectRatio_Key);
  auto pImageScaleBarLengthValue = filterArgs.value<int32>(k_ImageScaleBarLength_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  auto pEllipseFeatureAttributeMatrixPathValue = filterArgs.value<DataPath>(k_EllipseFeatureAttributeMatrixPath_Key);
  auto pCenterCoordinatesArrayNameValue = filterArgs.value<DataPath>(k_CenterCoordinatesArrayName_Key);
  auto pMajorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MajorAxisLengthArrayName_Key);
  auto pMinorAxisLengthArrayNameValue = filterArgs.value<DataPath>(k_MinorAxisLengthArrayName_Key);
  auto pRotationalAnglesArrayNameValue = filterArgs.value<DataPath>(k_RotationalAnglesArrayName_Key);
  auto pDetectedEllipsoidsFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_DetectedEllipsoidsFeatureIdsArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pEllipseFeatureAttributeMatrixPathValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS,
    // pDetectedEllipsoidsFeatureIdsArrayPathValue); resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> DetectEllipsoids::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinFiberAxisLengthValue = filterArgs.value<int32>(k_MinFiberAxisLength_Key);
  auto pMaxFiberAxisLengthValue = filterArgs.value<int32>(k_MaxFiberAxisLength_Key);
  auto pHoughTransformThresholdValue = filterArgs.value<float32>(k_HoughTransformThreshold_Key);
  auto pMinAspectRatioValue = filterArgs.value<float32>(k_MinAspectRatio_Key);
  auto pImageScaleBarLengthValue = filterArgs.value<int32>(k_ImageScaleBarLength_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
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

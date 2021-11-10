#include "FindShapes.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindShapes::name() const
{
  return FilterTraits<FindShapes>::name.str();
}

//------------------------------------------------------------------------------
std::string FindShapes::className() const
{
  return FilterTraits<FindShapes>::className;
}

//------------------------------------------------------------------------------
Uuid FindShapes::uuid() const
{
  return FilterTraits<FindShapes>::uuid;
}

//------------------------------------------------------------------------------
std::string FindShapes::humanName() const
{
  return "Find Feature Shapes";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindShapes::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindShapes::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeatureAttributeMatrixName_Key, "Cell Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Centroids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_Omega3sArrayName_Key, "Omega3s", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AxisLengthsArrayName_Key, "Axis Lengths", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AxisEulerAnglesArrayName_Key, "Axis Euler Angles", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AspectRatiosArrayName_Key, "Aspect Ratios", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VolumesArrayName_Key, "Volumes", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindShapes::clone() const
{
  return std::make_unique<FindShapes>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindShapes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pOmega3sArrayNameValue = filterArgs.value<DataPath>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayNameValue = filterArgs.value<DataPath>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayNameValue = filterArgs.value<DataPath>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindShapesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindShapes::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pOmega3sArrayNameValue = filterArgs.value<DataPath>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayNameValue = filterArgs.value<DataPath>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayNameValue = filterArgs.value<DataPath>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

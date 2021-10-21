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
Result<OutputActions> FindShapes::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixName_Key);
  auto pCentroidsArrayPathValue = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pOmega3sArrayNameValue = filterArgs.value<DataPath>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayNameValue = filterArgs.value<DataPath>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayNameValue = filterArgs.value<DataPath>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayNameValue = filterArgs.value<DataPath>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayNameValue = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindShapesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindShapes::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

#include "FindMinkowskiBouligandDimension.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::name() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::name.str();
}

//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::className() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::className;
}

//------------------------------------------------------------------------------
Uuid FindMinkowskiBouligandDimension::uuid() const
{
  return FilterTraits<FindMinkowskiBouligandDimension>::uuid;
}

//------------------------------------------------------------------------------
std::string FindMinkowskiBouligandDimension::humanName() const
{
  return "Find Minkowski-Bouligand Dimension";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindMinkowskiBouligandDimension::defaultTags() const
{
  return {"#Statistics", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters FindMinkowskiBouligandDimension::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_AttributeMatrixName_Key, "Fractal Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MinkowskiBouligandDimensionArrayName_Key, "Minkowski-Bouligand Dimension", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindMinkowskiBouligandDimension::clone() const
{
  return std::make_unique<FindMinkowskiBouligandDimension>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindMinkowskiBouligandDimension::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pMinkowskiBouligandDimensionArrayNameValue = filterArgs.value<DataPath>(k_MinkowskiBouligandDimensionArrayName_Key);

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
  auto action = std::make_unique<FindMinkowskiBouligandDimensionAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindMinkowskiBouligandDimension::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pAttributeMatrixNameValue = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  auto pMinkowskiBouligandDimensionArrayNameValue = filterArgs.value<DataPath>(k_MinkowskiBouligandDimensionArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

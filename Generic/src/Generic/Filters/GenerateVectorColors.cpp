#include "GenerateVectorColors.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateVectorColors::name() const
{
  return FilterTraits<GenerateVectorColors>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateVectorColors::className() const
{
  return FilterTraits<GenerateVectorColors>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateVectorColors::uuid() const
{
  return FilterTraits<GenerateVectorColors>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateVectorColors::humanName() const
{
  return "Generate Vector Colors";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateVectorColors::defaultTags() const
{
  return {"#Generic", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateVectorColors::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Apply to Good Voxels Only (Bad Voxels Will Be Black)", "", false));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_VectorsArrayPath_Key, "Vector Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellVectorColorsArrayName_Key, "Vector Colors", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateVectorColors::clone() const
{
  return std::make_unique<GenerateVectorColors>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateVectorColors::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pVectorsArrayPathValue = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellVectorColorsArrayNameValue = filterArgs.value<DataPath>(k_CellVectorColorsArrayName_Key);

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
  auto action = std::make_unique<GenerateVectorColorsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> GenerateVectorColors::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pVectorsArrayPathValue = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellVectorColorsArrayNameValue = filterArgs.value<DataPath>(k_CellVectorColorsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

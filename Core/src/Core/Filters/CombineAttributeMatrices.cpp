#include "CombineAttributeMatrices.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CombineAttributeMatrices::name() const
{
  return FilterTraits<CombineAttributeMatrices>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineAttributeMatrices::className() const
{
  return FilterTraits<CombineAttributeMatrices>::className;
}

//------------------------------------------------------------------------------
Uuid CombineAttributeMatrices::uuid() const
{
  return FilterTraits<CombineAttributeMatrices>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineAttributeMatrices::humanName() const
{
  return "Combine Feature/Ensemble Attribute Matrices";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineAttributeMatrices::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CombineAttributeMatrices::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Feature/Ensemble Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_FirstAttributeMatrixPath_Key, "First Feature/Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SecondAttributeMatrixPath_Key, "Second Feature/Ensemble Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FirstIndexArrayPath_Key, "First Index Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SecondIndexArrayPath_Key, "Second Index Array", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell/Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewIndexArrayName_Key, "New Index Array", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Feature/Ensemble Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CombinedAttributeMatrixName_Key, "Combined Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineAttributeMatrices::clone() const
{
  return std::make_unique<CombineAttributeMatrices>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineAttributeMatrices::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFirstAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FirstAttributeMatrixPath_Key);
  auto pSecondAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SecondAttributeMatrixPath_Key);
  auto pFirstIndexArrayPathValue = filterArgs.value<DataPath>(k_FirstIndexArrayPath_Key);
  auto pSecondIndexArrayPathValue = filterArgs.value<DataPath>(k_SecondIndexArrayPath_Key);
  auto pNewIndexArrayNameValue = filterArgs.value<DataPath>(k_NewIndexArrayName_Key);
  auto pCombinedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CombinedAttributeMatrixName_Key);

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
  auto action = std::make_unique<CombineAttributeMatricesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> CombineAttributeMatrices::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFirstAttributeMatrixPathValue = filterArgs.value<DataPath>(k_FirstAttributeMatrixPath_Key);
  auto pSecondAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SecondAttributeMatrixPath_Key);
  auto pFirstIndexArrayPathValue = filterArgs.value<DataPath>(k_FirstIndexArrayPath_Key);
  auto pSecondIndexArrayPathValue = filterArgs.value<DataPath>(k_SecondIndexArrayPath_Key);
  auto pNewIndexArrayNameValue = filterArgs.value<DataPath>(k_NewIndexArrayName_Key);
  auto pCombinedAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CombinedAttributeMatrixName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

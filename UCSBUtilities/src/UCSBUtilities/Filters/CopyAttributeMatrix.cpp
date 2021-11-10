#include "CopyAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CopyAttributeMatrix::name() const
{
  return FilterTraits<CopyAttributeMatrix>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyAttributeMatrix::className() const
{
  return FilterTraits<CopyAttributeMatrix>::className;
}

//------------------------------------------------------------------------------
Uuid CopyAttributeMatrix::uuid() const
{
  return FilterTraits<CopyAttributeMatrix>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyAttributeMatrix::humanName() const
{
  return "Copy Attribute Matrix";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyAttributeMatrix::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Attribute Matrix to Copy", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewAttributeMatrix_Key, "Copied Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyAttributeMatrix::clone() const
{
  return std::make_unique<CopyAttributeMatrix>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<DataPath>(k_NewAttributeMatrix_Key);

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
  auto action = std::make_unique<CopyAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> CopyAttributeMatrix::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<DataPath>(k_NewAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

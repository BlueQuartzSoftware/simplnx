#include "RenameAttributeMatrix.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RenameAttributeMatrix::name() const
{
  return FilterTraits<RenameAttributeMatrix>::name.str();
}

//------------------------------------------------------------------------------
std::string RenameAttributeMatrix::className() const
{
  return FilterTraits<RenameAttributeMatrix>::className;
}

//------------------------------------------------------------------------------
Uuid RenameAttributeMatrix::uuid() const
{
  return FilterTraits<RenameAttributeMatrix>::uuid;
}

//------------------------------------------------------------------------------
std::string RenameAttributeMatrix::humanName() const
{
  return "Rename Attribute Matrix";
}

//------------------------------------------------------------------------------
std::vector<std::string> RenameAttributeMatrix::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters RenameAttributeMatrix::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrixPath_Key, "Attribute Matrix to Rename", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_NewAttributeMatrix_Key, "New Attribute Matrix Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RenameAttributeMatrix::clone() const
{
  return std::make_unique<RenameAttributeMatrix>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RenameAttributeMatrix::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
  auto pNewAttributeMatrixValue = filterArgs.value<StringParameter::ValueType>(k_NewAttributeMatrix_Key);

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
  auto action = std::make_unique<RenameAttributeMatrixAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> RenameAttributeMatrix::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedAttributeMatrixPathValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  auto pNewAttributeMatrixValue = filterArgs.value<StringParameter::ValueType>(k_NewAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

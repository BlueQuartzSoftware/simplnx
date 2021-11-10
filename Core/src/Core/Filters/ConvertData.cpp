#include "ConvertData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertData::name() const
{
  return FilterTraits<ConvertData>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertData::className() const
{
  return FilterTraits<ConvertData>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertData::uuid() const
{
  return FilterTraits<ConvertData>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertData::humanName() const
{
  return "Convert AttributeArray Data Type";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertData::defaultTags() const
{
  return {"#Core", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "", NumericType::int8));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Convert", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayName_Key, "Converted Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertData::clone() const
{
  return std::make_unique<ConvertData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

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
  auto action = std::make_unique<ConvertDataAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ConvertData::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

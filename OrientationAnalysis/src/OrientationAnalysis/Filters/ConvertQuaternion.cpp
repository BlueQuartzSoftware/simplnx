#include "ConvertQuaternion.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertQuaternion::name() const
{
  return FilterTraits<ConvertQuaternion>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertQuaternion::className() const
{
  return FilterTraits<ConvertQuaternion>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertQuaternion::uuid() const
{
  return FilterTraits<ConvertQuaternion>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertQuaternion::humanName() const
{
  return "Convert Quaternion Order";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertQuaternion::defaultTags() const
{
  return {"#Processing", "#Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertQuaternion::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuaternionDataArrayPath_Key, "Quaternion Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Output Data Array Path", "", DataPath{}));
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertQuaternion::clone() const
{
  return std::make_unique<ConvertQuaternion>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertQuaternion::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

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
  auto action = std::make_unique<ConvertQuaternionAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ConvertQuaternion::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  auto pDeleteOriginalDataValue = filterArgs.value<bool>(k_DeleteOriginalData_Key);
  auto pConversionTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

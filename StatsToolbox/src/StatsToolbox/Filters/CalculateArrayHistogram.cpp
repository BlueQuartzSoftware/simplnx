#include "CalculateArrayHistogram.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CalculateArrayHistogram::name() const
{
  return FilterTraits<CalculateArrayHistogram>::name.str();
}

//------------------------------------------------------------------------------
std::string CalculateArrayHistogram::className() const
{
  return FilterTraits<CalculateArrayHistogram>::className;
}

//------------------------------------------------------------------------------
Uuid CalculateArrayHistogram::uuid() const
{
  return FilterTraits<CalculateArrayHistogram>::uuid;
}

//------------------------------------------------------------------------------
std::string CalculateArrayHistogram::humanName() const
{
  return "Calculate Frequency Histogram";
}

//------------------------------------------------------------------------------
std::vector<std::string> CalculateArrayHistogram::defaultTags() const
{
  return {"#Statistics", "#Ensemble"};
}

//------------------------------------------------------------------------------
Parameters CalculateArrayHistogram::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfBins_Key, "Number of Bins", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UserDefinedRange_Key, "Use Min & Max Range", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Min Value", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Max Value", "", 2.3456789));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_NewDataContainer_Key, "New Data Container", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Histogram", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerName_Key, "Data Container ", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewAttributeMatrixName_Key, "Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewDataArrayName_Key, "Histogram", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UserDefinedRange_Key, k_MinRange_Key, true);
  params.linkParameters(k_UserDefinedRange_Key, k_MaxRange_Key, true);
  params.linkParameters(k_NewDataContainer_Key, k_NewDataContainerName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CalculateArrayHistogram::clone() const
{
  return std::make_unique<CalculateArrayHistogram>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CalculateArrayHistogram::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pUserDefinedRangeValue = filterArgs.value<bool>(k_UserDefinedRange_Key);
  auto pMinRangeValue = filterArgs.value<float64>(k_MinRange_Key);
  auto pMaxRangeValue = filterArgs.value<float64>(k_MaxRange_Key);
  auto pNewDataContainerValue = filterArgs.value<bool>(k_NewDataContainer_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pNewAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewAttributeMatrixName_Key);
  auto pNewDataArrayNameValue = filterArgs.value<DataPath>(k_NewDataArrayName_Key);

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
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pNewDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CalculateArrayHistogram::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNumberOfBinsValue = filterArgs.value<int32>(k_NumberOfBins_Key);
  auto pUserDefinedRangeValue = filterArgs.value<bool>(k_UserDefinedRange_Key);
  auto pMinRangeValue = filterArgs.value<float64>(k_MinRange_Key);
  auto pMaxRangeValue = filterArgs.value<float64>(k_MaxRange_Key);
  auto pNewDataContainerValue = filterArgs.value<bool>(k_NewDataContainer_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  auto pNewAttributeMatrixNameValue = filterArgs.value<DataPath>(k_NewAttributeMatrixName_Key);
  auto pNewDataArrayNameValue = filterArgs.value<DataPath>(k_NewDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

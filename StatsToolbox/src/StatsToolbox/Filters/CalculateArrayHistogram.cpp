#include "CalculateArrayHistogram.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string CalculateArrayHistogram::name() const
{
  return FilterTraits<CalculateArrayHistogram>::name.str();
}

std::string CalculateArrayHistogram::className() const
{
  return FilterTraits<CalculateArrayHistogram>::className;
}

Uuid CalculateArrayHistogram::uuid() const
{
  return FilterTraits<CalculateArrayHistogram>::uuid;
}

std::string CalculateArrayHistogram::humanName() const
{
  return "Calculate Frequency Histogram";
}

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

IFilter::UniquePointer CalculateArrayHistogram::clone() const
{
  return std::make_unique<CalculateArrayHistogram>();
}

Result<OutputActions> CalculateArrayHistogram::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
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

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CalculateArrayHistogramAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CalculateArrayHistogram::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

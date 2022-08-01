#include "FindNeighborListStatistics.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNeighborListStatistics::name() const
{
  return FilterTraits<FindNeighborListStatistics>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNeighborListStatistics::className() const
{
  return FilterTraits<FindNeighborListStatistics>::className;
}

//------------------------------------------------------------------------------
Uuid FindNeighborListStatistics::uuid() const
{
  return FilterTraits<FindNeighborListStatistics>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNeighborListStatistics::humanName() const
{
  return "Find NeighborList Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighborListStatistics::defaultTags() const
{
  return {"#DREAM3DReview", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindNeighborListStatistics::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMin_Key, "Find Minimum", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMax_Key, "Find Maximum", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindStdDeviation_Key, "Find Standard Deviation", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestinationAttributeMatrix_Key, "Destination Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_LengthArrayName_Key, "Length", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MinimumArrayName_Key, "Minimum", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MaximumArrayName_Key, "Maximum", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MeanArrayName_Key, "Mean", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MedianArrayName_Key, "Median", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_StdDeviationArrayName_Key, "Standard Deviation", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SummationArrayName_Key, "Summation", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindLength_Key, k_LengthArrayName_Key, true);
  params.linkParameters(k_FindMin_Key, k_MinimumArrayName_Key, true);
  params.linkParameters(k_FindMax_Key, k_MaximumArrayName_Key, true);
  params.linkParameters(k_FindMean_Key, k_MeanArrayName_Key, true);
  params.linkParameters(k_FindMedian_Key, k_MedianArrayName_Key, true);
  params.linkParameters(k_FindStdDeviation_Key, k_StdDeviationArrayName_Key, true);
  params.linkParameters(k_FindSummation_Key, k_SummationArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighborListStatistics::clone() const
{
  return std::make_unique<FindNeighborListStatistics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighborListStatistics::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pFindLengthValue = filterArgs.value<bool>(k_FindLength_Key);
  auto pFindMinValue = filterArgs.value<bool>(k_FindMin_Key);
  auto pFindMaxValue = filterArgs.value<bool>(k_FindMax_Key);
  auto pFindMeanValue = filterArgs.value<bool>(k_FindMean_Key);
  auto pFindMedianValue = filterArgs.value<bool>(k_FindMedian_Key);
  auto pFindStdDeviationValue = filterArgs.value<bool>(k_FindStdDeviation_Key);
  auto pFindSummationValue = filterArgs.value<bool>(k_FindSummation_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pLengthArrayNameValue = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  auto pMinimumArrayNameValue = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  auto pMaximumArrayNameValue = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  auto pMeanArrayNameValue = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  auto pMedianArrayNameValue = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  auto pStdDeviationArrayNameValue = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  auto pSummationArrayNameValue = filterArgs.value<DataPath>(k_SummationArrayName_Key);

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

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindNeighborListStatistics::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFindLengthValue = filterArgs.value<bool>(k_FindLength_Key);
  auto pFindMinValue = filterArgs.value<bool>(k_FindMin_Key);
  auto pFindMaxValue = filterArgs.value<bool>(k_FindMax_Key);
  auto pFindMeanValue = filterArgs.value<bool>(k_FindMean_Key);
  auto pFindMedianValue = filterArgs.value<bool>(k_FindMedian_Key);
  auto pFindStdDeviationValue = filterArgs.value<bool>(k_FindStdDeviation_Key);
  auto pFindSummationValue = filterArgs.value<bool>(k_FindSummation_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pLengthArrayNameValue = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  auto pMinimumArrayNameValue = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  auto pMaximumArrayNameValue = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  auto pMeanArrayNameValue = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  auto pMedianArrayNameValue = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  auto pStdDeviationArrayNameValue = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  auto pSummationArrayNameValue = filterArgs.value<DataPath>(k_SummationArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

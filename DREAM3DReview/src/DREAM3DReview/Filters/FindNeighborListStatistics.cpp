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
  return {"#DREAM3D Review", "#Statistics"};
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "", DataPath{}));
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
IFilter::PreflightResult FindNeighborListStatistics::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindNeighborListStatisticsAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindNeighborListStatistics::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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

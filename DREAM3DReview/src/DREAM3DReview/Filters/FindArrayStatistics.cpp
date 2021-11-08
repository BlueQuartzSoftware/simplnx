#include "FindArrayStatistics.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindArrayStatistics::name() const
{
  return FilterTraits<FindArrayStatistics>::name.str();
}

//------------------------------------------------------------------------------
std::string FindArrayStatistics::className() const
{
  return FilterTraits<FindArrayStatistics>::className;
}

//------------------------------------------------------------------------------
Uuid FindArrayStatistics::uuid() const
{
  return FilterTraits<FindArrayStatistics>::uuid;
}

//------------------------------------------------------------------------------
std::string FindArrayStatistics::humanName() const
{
  return "Find Attribute Array Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindArrayStatistics::defaultTags() const
{
  return {"#DREAM3D Review", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindArrayStatistics::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Statistics Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindHistogram_Key, "Find Histogram", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Histogram Min Value", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Histogram Max Value", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_UseFullRange_Key, "Use Full Range for Histogram", "", false));
  params.insert(std::make_unique<Int32Parameter>(k_NumBins_Key, "Number of Bins", "", 1234356));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMin_Key, "Find Minimum", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMax_Key, "Find Maximum", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindStdDeviation_Key, "Find Standard Deviation", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "", false));
  params.insertSeparator(Parameters::Separator{"Algorithm Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ComputeByIndex_Key, "Compute Statistics Per Feature/Ensemble", "", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StandardizeData_Key, "Standardize Data", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DestinationAttributeMatrix_Key, "Destination Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_HistogramArrayName_Key, "Histogram", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_LengthArrayName_Key, "Length", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MinimumArrayName_Key, "Minimum", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MaximumArrayName_Key, "Maximum", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MeanArrayName_Key, "Mean", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_MedianArrayName_Key, "Median", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_StdDeviationArrayName_Key, "Standard Deviation", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SummationArrayName_Key, "Summation", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_StandardizedArrayName_Key, "Standardized Data", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindHistogram_Key, k_HistogramArrayName_Key, true);
  params.linkParameters(k_FindHistogram_Key, k_UseFullRange_Key, true);
  params.linkParameters(k_FindHistogram_Key, k_NumBins_Key, true);
  params.linkParameters(k_FindHistogram_Key, k_MinRange_Key, true);
  params.linkParameters(k_FindHistogram_Key, k_MaxRange_Key, true);
  params.linkParameters(k_FindLength_Key, k_LengthArrayName_Key, true);
  params.linkParameters(k_FindMin_Key, k_MinimumArrayName_Key, true);
  params.linkParameters(k_FindMax_Key, k_MaximumArrayName_Key, true);
  params.linkParameters(k_FindMean_Key, k_MeanArrayName_Key, true);
  params.linkParameters(k_FindMedian_Key, k_MedianArrayName_Key, true);
  params.linkParameters(k_FindStdDeviation_Key, k_StdDeviationArrayName_Key, true);
  params.linkParameters(k_FindSummation_Key, k_SummationArrayName_Key, true);
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_ComputeByIndex_Key, k_FeatureIdsArrayPath_Key, true);
  params.linkParameters(k_StandardizeData_Key, k_StandardizedArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindArrayStatistics::clone() const
{
  return std::make_unique<FindArrayStatistics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindArrayStatistics::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFindHistogramValue = filterArgs.value<bool>(k_FindHistogram_Key);
  auto pMinRangeValue = filterArgs.value<float64>(k_MinRange_Key);
  auto pMaxRangeValue = filterArgs.value<float64>(k_MaxRange_Key);
  auto pUseFullRangeValue = filterArgs.value<bool>(k_UseFullRange_Key);
  auto pNumBinsValue = filterArgs.value<int32>(k_NumBins_Key);
  auto pFindLengthValue = filterArgs.value<bool>(k_FindLength_Key);
  auto pFindMinValue = filterArgs.value<bool>(k_FindMin_Key);
  auto pFindMaxValue = filterArgs.value<bool>(k_FindMax_Key);
  auto pFindMeanValue = filterArgs.value<bool>(k_FindMean_Key);
  auto pFindMedianValue = filterArgs.value<bool>(k_FindMedian_Key);
  auto pFindStdDeviationValue = filterArgs.value<bool>(k_FindStdDeviation_Key);
  auto pFindSummationValue = filterArgs.value<bool>(k_FindSummation_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pComputeByIndexValue = filterArgs.value<bool>(k_ComputeByIndex_Key);
  auto pStandardizeDataValue = filterArgs.value<bool>(k_StandardizeData_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pHistogramArrayNameValue = filterArgs.value<DataPath>(k_HistogramArrayName_Key);
  auto pLengthArrayNameValue = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  auto pMinimumArrayNameValue = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  auto pMaximumArrayNameValue = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  auto pMeanArrayNameValue = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  auto pMedianArrayNameValue = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  auto pStdDeviationArrayNameValue = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  auto pSummationArrayNameValue = filterArgs.value<DataPath>(k_SummationArrayName_Key);
  auto pStandardizedArrayNameValue = filterArgs.value<DataPath>(k_StandardizedArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindArrayStatisticsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindArrayStatistics::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFindHistogramValue = filterArgs.value<bool>(k_FindHistogram_Key);
  auto pMinRangeValue = filterArgs.value<float64>(k_MinRange_Key);
  auto pMaxRangeValue = filterArgs.value<float64>(k_MaxRange_Key);
  auto pUseFullRangeValue = filterArgs.value<bool>(k_UseFullRange_Key);
  auto pNumBinsValue = filterArgs.value<int32>(k_NumBins_Key);
  auto pFindLengthValue = filterArgs.value<bool>(k_FindLength_Key);
  auto pFindMinValue = filterArgs.value<bool>(k_FindMin_Key);
  auto pFindMaxValue = filterArgs.value<bool>(k_FindMax_Key);
  auto pFindMeanValue = filterArgs.value<bool>(k_FindMean_Key);
  auto pFindMedianValue = filterArgs.value<bool>(k_FindMedian_Key);
  auto pFindStdDeviationValue = filterArgs.value<bool>(k_FindStdDeviation_Key);
  auto pFindSummationValue = filterArgs.value<bool>(k_FindSummation_Key);
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pComputeByIndexValue = filterArgs.value<bool>(k_ComputeByIndex_Key);
  auto pStandardizeDataValue = filterArgs.value<bool>(k_StandardizeData_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  auto pHistogramArrayNameValue = filterArgs.value<DataPath>(k_HistogramArrayName_Key);
  auto pLengthArrayNameValue = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  auto pMinimumArrayNameValue = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  auto pMaximumArrayNameValue = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  auto pMeanArrayNameValue = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  auto pMedianArrayNameValue = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  auto pStdDeviationArrayNameValue = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  auto pSummationArrayNameValue = filterArgs.value<DataPath>(k_SummationArrayName_Key);
  auto pStandardizedArrayNameValue = filterArgs.value<DataPath>(k_StandardizedArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex

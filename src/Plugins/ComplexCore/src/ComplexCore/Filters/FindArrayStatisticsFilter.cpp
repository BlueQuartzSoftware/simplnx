#include "FindArrayStatisticsFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/Filters/Algorithms/FindArrayStatistics.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindArrayStatisticsFilter::name() const
{
  return FilterTraits<FindArrayStatisticsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindArrayStatisticsFilter::className() const
{
  return FilterTraits<FindArrayStatisticsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindArrayStatisticsFilter::uuid() const
{
  return FilterTraits<FindArrayStatisticsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindArrayStatisticsFilter::humanName() const
{
  return "Find Attribute Array Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindArrayStatisticsFilter::defaultTags() const
{
  return {"#DREAM3D Review", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindArrayStatisticsFilter::parameters() const
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
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "", DataPath{}, ArraySelectionParameter::AllowedTypes{}));
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
IFilter::UniquePointer FindArrayStatisticsFilter::clone() const
{
  return std::make_unique<FindArrayStatisticsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindArrayStatisticsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
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
Result<> FindArrayStatisticsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  FindArrayStatisticsInputValues inputValues;

  inputValues.FindHistogram = filterArgs.value<bool>(k_FindHistogram_Key);
  inputValues.MinRange = filterArgs.value<float64>(k_MinRange_Key);
  inputValues.MaxRange = filterArgs.value<float64>(k_MaxRange_Key);
  inputValues.UseFullRange = filterArgs.value<bool>(k_UseFullRange_Key);
  inputValues.NumBins = filterArgs.value<int32>(k_NumBins_Key);
  inputValues.FindLength = filterArgs.value<bool>(k_FindLength_Key);
  inputValues.FindMin = filterArgs.value<bool>(k_FindMin_Key);
  inputValues.FindMax = filterArgs.value<bool>(k_FindMax_Key);
  inputValues.FindMean = filterArgs.value<bool>(k_FindMean_Key);
  inputValues.FindMedian = filterArgs.value<bool>(k_FindMedian_Key);
  inputValues.FindStdDeviation = filterArgs.value<bool>(k_FindStdDeviation_Key);
  inputValues.FindSummation = filterArgs.value<bool>(k_FindSummation_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.ComputeByIndex = filterArgs.value<bool>(k_ComputeByIndex_Key);
  inputValues.StandardizeData = filterArgs.value<bool>(k_StandardizeData_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  inputValues.HistogramArrayName = filterArgs.value<DataPath>(k_HistogramArrayName_Key);
  inputValues.LengthArrayName = filterArgs.value<DataPath>(k_LengthArrayName_Key);
  inputValues.MinimumArrayName = filterArgs.value<DataPath>(k_MinimumArrayName_Key);
  inputValues.MaximumArrayName = filterArgs.value<DataPath>(k_MaximumArrayName_Key);
  inputValues.MeanArrayName = filterArgs.value<DataPath>(k_MeanArrayName_Key);
  inputValues.MedianArrayName = filterArgs.value<DataPath>(k_MedianArrayName_Key);
  inputValues.StdDeviationArrayName = filterArgs.value<DataPath>(k_StdDeviationArrayName_Key);
  inputValues.SummationArrayName = filterArgs.value<DataPath>(k_SummationArrayName_Key);
  inputValues.StandardizedArrayName = filterArgs.value<DataPath>(k_StandardizedArrayName_Key);

  return FindArrayStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

#include "FindArrayStatisticsFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/Math/StatisticsCalculations.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include "ComplexCore/Filters/Algorithms/FindArrayStatistics.hpp"

using namespace complex;

namespace complex
{
OutputActions FindArrayStatisticsFilter::createCompatibleArrays(const DataStructure& data, const Arguments& args, usize numBins, std::vector<usize> tupleDims) const
{
  auto findLength = args.value<bool>(k_FindLength_Key);
  auto findMin = args.value<bool>(k_FindMin_Key);
  auto findMax = args.value<bool>(k_FindMax_Key);
  auto findMean = args.value<bool>(k_FindMean_Key);
  auto findMedian = args.value<bool>(k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(k_FindStdDeviation_Key);
  auto findSummation = args.value<bool>(k_FindSummation_Key);
  auto findHistogramValue = args.value<bool>(k_FindHistogram_Key);
  auto minRangeValue = args.value<float64>(k_MinRange_Key);
  auto maxRangeValue = args.value<float64>(k_MaxRange_Key);
  auto useFullRangeValue = args.value<bool>(k_UseFullRange_Key);
  auto computeByIndexValue = args.value<bool>(k_ComputeByIndex_Key);
  auto standardizeDataValue = args.value<bool>(k_StandardizeData_Key);
  auto inputArrayPath = args.value<DataPath>(k_SelectedArrayPath_Key);
  auto* inputArray = data.getDataAs<IDataArray>(inputArrayPath);
  auto destinationAttributeMatrixValue = args.value<DataPath>(k_DestinationAttributeMatrix_Key);
  DataType dataType = inputArray->getDataType();

  OutputActions actions;

  auto amAction = std::make_unique<CreateAttributeMatrixAction>(destinationAttributeMatrixValue, tupleDims);
  actions.actions.push_back(std::move(amAction));

  if(findLength)
  {
    auto arrayPath = args.value<std::string>(k_LengthArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findMin)
  {
    auto arrayPath = args.value<std::string>(k_MinimumArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findMax)
  {
    auto arrayPath = args.value<std::string>(k_MaximumArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findMean)
  {
    auto arrayPath = args.value<std::string>(k_MeanArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findMedian)
  {
    auto arrayPath = args.value<std::string>(k_MedianArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findStdDeviation)
  {
    auto arrayPath = args.value<std::string>(k_StdDeviationArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findSummation)
  {
    auto arrayPath = args.value<std::string>(k_SummationArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(findHistogramValue)
  {
    auto arrayPath = args.value<std::string>(k_HistogramArrayName_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{numBins}, destinationAttributeMatrixValue.createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }
  if(standardizeDataValue)
  {
    auto arrayPath = args.value<std::string>(k_StandardizedArrayName_Key);
    auto action =
        std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{inputArray->getNumberOfTuples()}, std::vector<usize>{1}, inputArrayPath.getParent().createChildPath(arrayPath));
    actions.actions.push_back(std::move(action));
  }

  return std::move(actions);
}

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
  return {"#ComplexCore", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindArrayStatisticsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Compute Statistics", "Input Attribute Array for which to compute statistics", DataPath{},
                                                          complex::GetAllDataTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_DestinationAttributeMatrix_Key, "Destination Attribute Matrix", "Attribute Matrix in which to store the computed statistics", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Histogram Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindHistogram_Key, "Find Histogram", "Whether to compute the histogram of the input array", false));
  params.insert(std::make_unique<Float64Parameter>(k_MinRange_Key, "Histogram Min Value", "Min cutoff value for histogram", 0.0));
  params.insert(std::make_unique<Float64Parameter>(k_MaxRange_Key, "Histogram Max Value", "Max cutoff value for histogram", 0.0));
  params.insert(
      std::make_unique<BoolParameter>(k_UseFullRange_Key, "Use Full Range for Histogram", "If true, ignore min and max and use min and max from array upon which histogram is computed", false));
  params.insert(std::make_unique<Int32Parameter>(k_NumBins_Key, "Number of Bins", "Number of bins in histogram", 0));
  params.insert(std::make_unique<StringParameter>(k_HistogramArrayName_Key, "Histogram Array Name", "The name of the histogram array", "Historgram"));

  params.insertSeparator(Parameters::Separator{"Statistics Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "Whether to compute the length of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_LengthArrayName_Key, "Length Array Name", "The name of the length array", "Length"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMin_Key, "Find Minimum", "Whether to compute the minimum of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_MinimumArrayName_Key, "Minimum Array Name", "The name of the minimum array", "Minimum"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMax_Key, "Find Maximum", "Whether to compute the maximum of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_MaximumArrayName_Key, "Maximum Array Name", "The name of the maximum array", "Maximum"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "Whether to compute the arithmetic mean of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_MeanArrayName_Key, "Mean Array Name", "The name of the mean array", "Mean"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "Whether to compute the median of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_MedianArrayName_Key, "Median Array Name", "The name of the median array", "Median"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindStdDeviation_Key, "Find Standard Deviation", "Whether to compute the standard deviation of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_StdDeviationArrayName_Key, "Standard Deviation Array Name", "The name of the standard deviation array", "StandardDeviation"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "Whether to compute the summation of the input array", false));
  params.insert(std::make_unique<StringParameter>(k_SummationArrayName_Key, "Summation Array Name", "The name of the summation array", "Summation"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StandardizeData_Key, "Standardize Data", "", false));
  params.insert(std::make_unique<StringParameter>(k_StandardizedArrayName_Key, "Standardized Data Array Name", "The name of the standardized data array", "Standardized"));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Whether to use a boolean mask array to ignore certain points flagged as false from the statistics", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "The path to the data array that specifies if the point is to be counted in the statistics", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Algorithm Options"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ComputeByIndex_Key, "Compute Statistics Per Feature/Ensemble", "Whether the statistics should be computed on a Feature/Ensemble basis", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

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
  params.linkParameters(k_ComputeByIndex_Key, k_CellFeatureIdsArrayPath_Key, true);
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
  auto pFindHistogramValue = filterArgs.value<bool>(k_FindHistogram_Key);
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
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pDestinationAttributeMatrixValue = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!pFindHistogramValue && !pFindMinValue && !pFindMaxValue && !pFindMeanValue && !pFindMedianValue && !pFindStdDeviationValue && !pFindSummationValue && !pFindLengthValue)
  {
    return {ConvertResultTo<OutputActions>(MakeWarningVoidResult(-57200, "No statistics have been selected, so this filter will perform no operations"), {})};
  }

  usize numBins = 0;
  if(pNumBinsValue < 0)
  {
    resultOutputActions.warnings().push_back({-57201, "Value entered for number of bins is beyond the representable range for a 32 bit integer. The filter will use the default value of 0."});
  }
  numBins = static_cast<usize>(pNumBinsValue);

  std::vector<DataPath> inputDataArrayPaths;

  const auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(pSelectedArrayPathValue);

  if(inputArrayPtr == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-57202, fmt::format("Could not find selected input array at path '{}' ", pSelectedArrayPathValue.toString())), {}};
  }

  inputDataArrayPaths.push_back(pSelectedArrayPathValue);

  if(inputArrayPtr->getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(-57203, fmt::format("Input array must be a scalar array")), {}};
  }

  AttributeMatrix::ShapeType tupleDims = {1};
  const Int32Array* featureIdsPtr = nullptr;
  if(pComputeByIndexValue)
  {
    auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
    featureIdsPtr = dataStructure.getDataAs<Int32Array>(pFeatureIdsArrayPathValue);
    if(featureIdsPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-57205, fmt::format("Could not find feature ids array at path '{}' ", pFeatureIdsArrayPathValue.toString())), {}};
    }
    inputDataArrayPaths.push_back(pFeatureIdsArrayPathValue);

    usize numFeatures = FindArrayStatistics::FindNumFeatures(*featureIdsPtr);
    tupleDims = {numFeatures};
  }

  if(pUseMaskValue)
  {
    const auto* maskPtr = dataStructure.getDataAs<IDataArray>(pMaskArrayPathValue);
    if(maskPtr == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Could not find mask array at path '{}' ", pMaskArrayPathValue.toString())), {}};
    }
    if(maskPtr->getDataType() != DataType::boolean && maskPtr->getDataType() != DataType::uint8)
    {
      return {MakeErrorResult<OutputActions>(-57207, fmt::format("Mask array must be of type Boolean or UInt8")), {}};
    }
    inputDataArrayPaths.push_back(pMaskArrayPathValue);
  }

  if(pStandardizeDataValue)
  {
    if(!pFindMeanValue || !pFindStdDeviationValue)
    {
      return {MakeErrorResult<OutputActions>(-57208, fmt::format("To standardize data, the \"Find Mean\" and \"Find Standard Deviation\" options must also be checked")), {}};
    }
  }

  if(!dataStructure.validateNumberOfTuples(inputDataArrayPaths))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-57209, "Input arrays do not have matching tuple counts."}})};
  }

  resultOutputActions.value().actions = createCompatibleArrays(dataStructure, filterArgs, numBins, tupleDims).actions;

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
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  inputValues.HistogramArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_HistogramArrayName_Key));
  inputValues.LengthArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_LengthArrayName_Key));
  inputValues.MinimumArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_MinimumArrayName_Key));
  inputValues.MaximumArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_MaximumArrayName_Key));
  inputValues.MeanArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_MeanArrayName_Key));
  inputValues.MedianArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_MedianArrayName_Key));
  inputValues.StdDeviationArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_StdDeviationArrayName_Key));
  inputValues.SummationArrayName = inputValues.DestinationAttributeMatrix.createChildPath(filterArgs.value<std::string>(k_SummationArrayName_Key));
  inputValues.StandardizedArrayName = inputValues.SelectedArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_StandardizedArrayName_Key));

  return FindArrayStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex

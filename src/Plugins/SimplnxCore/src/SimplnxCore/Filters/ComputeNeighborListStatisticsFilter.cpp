#include "ComputeNeighborListStatisticsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeNeighborListStatistics.hpp"

#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NeighborListSelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
constexpr int64 k_NoAction = -6800;
constexpr int64 k_MissingInputArray = -6801;

//------------------------------------------------------------------------------
OutputActions CreateCompatibleArrays(const DataStructure& dataStructure, const Arguments& args)
{
  auto findLength = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindLength_Key);
  auto findMin = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindMinimum_Key);
  auto findMax = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindMaximum_Key);
  auto findMean = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindMean_Key);
  auto findMedian = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindStandardDeviation_Key);
  auto findSummation = args.value<bool>(ComputeNeighborListStatisticsFilter::k_FindSummation_Key);

  auto inputArrayPath = args.value<DataPath>(ComputeNeighborListStatisticsFilter::k_InputNeighborListPath_Key);
  auto* inputArray = dataStructure.getDataAs<INeighborList>(inputArrayPath);
  std::vector<usize> tupleDims{inputArray->getNumberOfTuples()};
  DataType dataType = inputArray->getDataType();
  const DataPath outputGroupPath = inputArrayPath.getParent();

  OutputActions actions;
  if(findLength)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_LengthName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMin)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_MinimumName_Key));
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMax)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_MaximumName_Key));
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMean)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_MeanName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMedian)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_MedianName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findStdDeviation)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_StandardDeviationName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findSummation)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(ComputeNeighborListStatisticsFilter::k_SummationName_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }

  return std::move(actions);
}
} // namespace

//------------------------------------------------------------------------------
std::string ComputeNeighborListStatisticsFilter::name() const
{
  return FilterTraits<ComputeNeighborListStatisticsFilter>::name;
}

//------------------------------------------------------------------------------
std::string ComputeNeighborListStatisticsFilter::className() const
{
  return FilterTraits<ComputeNeighborListStatisticsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeNeighborListStatisticsFilter::uuid() const
{
  return FilterTraits<ComputeNeighborListStatisticsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeNeighborListStatisticsFilter::humanName() const
{
  return "Compute Neighbor List Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeNeighborListStatisticsFilter::defaultTags() const
{
  return {className(), "NeighborList", "Statistics", "Analytics", "Find", "Generate", "Calculate", "Determine"};
}

//------------------------------------------------------------------------------
Parameters ComputeNeighborListStatisticsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "Specifies whether or not the filter creates the Length array during calculations", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMinimum_Key, "Find Minimum", "Specifies whether or not the filter creates the Minimum array during calculations", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMaximum_Key, "Find Maximum", "Specifies whether or not the filter creates the Maximum array during calculations", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "Specifies whether or not the filter creates the Mean array during calculations", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "Specifies whether or not the filter creates the Median array during calculations", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_FindStandardDeviation_Key, "Find Standard Deviation", "Specifies whether or not the filter creates the Standard Deviation array during calculations", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "Specifies whether or not the filter creates the Summation array during calculations", true));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<NeighborListSelectionParameter>(k_InputNeighborListPath_Key, "NeighborList to Compute Statistics", "Input Data Array to compute statistics", DataPath(),
                                                                 nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_LengthName_Key, "Length", "Path to create the Length array during calculations", "Length"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MinimumName_Key, "Minimum", "Path to create the Minimum array during calculations", "Minimum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaximumName_Key, "Maximum", "Path to create the Maximum array during calculations", "Maximum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MeanName_Key, "Mean", "Path to create the Mean array during calculations", "Mean"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MedianName_Key, "Median", "Path to create the Median array during calculations", "Median"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_StandardDeviationName_Key, "Standard Deviation", "Path to create the Standard Deviation array during calculations", "StandardDeviation"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SummationName_Key, "Summation", "Path to create the Summation array during calculations", "Summation"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_FindLength_Key, k_LengthName_Key, true);
  params.linkParameters(k_FindMinimum_Key, k_MinimumName_Key, true);
  params.linkParameters(k_FindMaximum_Key, k_MaximumName_Key, true);
  params.linkParameters(k_FindMean_Key, k_MeanName_Key, true);
  params.linkParameters(k_FindMedian_Key, k_MedianName_Key, true);
  params.linkParameters(k_FindStandardDeviation_Key, k_StandardDeviationName_Key, true);
  params.linkParameters(k_FindSummation_Key, k_SummationName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeNeighborListStatisticsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeNeighborListStatisticsFilter::clone() const
{
  return std::make_unique<ComputeNeighborListStatisticsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeNeighborListStatisticsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto findLength = args.value<bool>(k_FindLength_Key);
  auto findMin = args.value<bool>(k_FindMinimum_Key);
  auto findMax = args.value<bool>(k_FindMaximum_Key);
  auto findMean = args.value<bool>(k_FindMean_Key);
  auto findMedian = args.value<bool>(k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(k_FindStandardDeviation_Key);
  auto findSummation = args.value<bool>(k_FindSummation_Key);

  auto inputArrayPath = args.value<DataPath>(k_InputNeighborListPath_Key);

  if(!findMin && !findMax && !findMean && !findMedian && !findStdDeviation && !findSummation && !findLength)
  {
    return MakePreflightErrorResult(k_NoAction, "No statistics have been selected");
  }

  std::vector<DataPath> dataArrayPaths;

  auto inputArray = dataStructure.getDataAs<INeighborList>(inputArrayPath);
  if(inputArray == nullptr)
  {
    return MakePreflightErrorResult(k_MissingInputArray, "Missing input array");
  }

  dataArrayPaths.push_back(inputArrayPath);

  return {std::move(CreateCompatibleArrays(dataStructure, args))};
}

//------------------------------------------------------------------------------
Result<> ComputeNeighborListStatisticsFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  ComputeNeighborListStatisticsInputValues inputValues;

  inputValues.FindLength = args.value<bool>(k_FindLength_Key);
  inputValues.FindMin = args.value<bool>(k_FindMinimum_Key);
  inputValues.FindMax = args.value<bool>(k_FindMaximum_Key);
  inputValues.FindMean = args.value<bool>(k_FindMean_Key);
  inputValues.FindMedian = args.value<bool>(k_FindMedian_Key);
  inputValues.FindStdDeviation = args.value<bool>(k_FindStandardDeviation_Key);
  inputValues.FindSummation = args.value<bool>(k_FindSummation_Key);

  if(!inputValues.FindMin && !inputValues.FindMax && !inputValues.FindMean && !inputValues.FindMedian && !inputValues.FindStdDeviation && !inputValues.FindSummation && !inputValues.FindLength)
  {
    return {};
  }

  inputValues.TargetNeighborListPath = args.value<DataPath>(k_InputNeighborListPath_Key);
  const DataPath outputGroupPath = inputValues.TargetNeighborListPath.getParent();

  if(inputValues.FindLength)
  {
    inputValues.LengthPath = outputGroupPath.createChildPath(args.value<std::string>(k_LengthName_Key));
  }
  if(inputValues.FindMin)
  {
    inputValues.MinPath = outputGroupPath.createChildPath(args.value<std::string>(k_MinimumName_Key));
  }
  if(inputValues.FindMax)
  {
    inputValues.MaxPath = outputGroupPath.createChildPath(args.value<std::string>(k_MaximumName_Key));
  }
  if(inputValues.FindMean)
  {
    inputValues.MeanPath = outputGroupPath.createChildPath(args.value<std::string>(k_MeanName_Key));
  }
  if(inputValues.FindMedian)
  {
    inputValues.MedianPath = outputGroupPath.createChildPath(args.value<std::string>(k_MedianName_Key));
  }
  if(inputValues.FindStdDeviation)
  {
    inputValues.StdDeviationPath = outputGroupPath.createChildPath(args.value<std::string>(k_StandardDeviationName_Key));
  }
  if(inputValues.FindSummation)
  {
    inputValues.SummationPath = outputGroupPath.createChildPath(args.value<std::string>(k_SummationName_Key));
  }

  return ComputeNeighborListStatistics(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FindLengthKey = "FindLength";
constexpr StringLiteral k_FindMinKey = "FindMin";
constexpr StringLiteral k_FindMaxKey = "FindMax";
constexpr StringLiteral k_FindMeanKey = "FindMean";
constexpr StringLiteral k_FindMedianKey = "FindMedian";
constexpr StringLiteral k_FindStdDeviationKey = "FindStdDeviation";
constexpr StringLiteral k_FindSummationKey = "FindSummation";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_DestinationAttributeMatrixKey = "DestinationAttributeMatrix";
constexpr StringLiteral k_LengthArrayNameKey = "LengthArrayName";
constexpr StringLiteral k_MinimumArrayNameKey = "MinimumArrayName";
constexpr StringLiteral k_MaximumArrayNameKey = "MaximumArrayName";
constexpr StringLiteral k_MeanArrayNameKey = "MeanArrayName";
constexpr StringLiteral k_MedianArrayNameKey = "MedianArrayName";
constexpr StringLiteral k_StdDeviationArrayNameKey = "StdDeviationArrayName";
constexpr StringLiteral k_SummationArrayNameKey = "SummationArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeNeighborListStatisticsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeNeighborListStatisticsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindLengthKey, k_FindLength_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMinKey, k_FindMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMaxKey, k_FindMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMeanKey, k_FindMean_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMedianKey, k_FindMedian_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindStdDeviationKey, k_FindStandardDeviation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindSummationKey, k_FindSummation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_InputNeighborListPath_Key));
  // No destination attribute matrix parameter in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_LengthArrayNameKey, k_LengthName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MinimumArrayNameKey, k_MinimumName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MaximumArrayNameKey, k_MaximumName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MeanArrayNameKey, k_MeanName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MedianArrayNameKey, k_MedianName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_StdDeviationArrayNameKey, k_StandardDeviationName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SummationArrayNameKey, k_SummationName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core

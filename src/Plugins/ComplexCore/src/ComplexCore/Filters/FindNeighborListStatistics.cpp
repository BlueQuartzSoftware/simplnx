#include "FindNeighborListStatistics.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NeighborListSelectionParameter.hpp"
#include "complex/Utilities/Math/StatisticsCalculations.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include "complex/Utilities/ParallelDataAlgorithm.hpp"

namespace complex
{
namespace
{
constexpr int64 k_NoAction = -6800;
constexpr int64 k_MissingInputArray = -6801;
constexpr int64 k_BoolTypeNeighborList = -6802;
constexpr int64 k_EmptyNeighborList = -6803;

template <typename T>
class FindNeighborListStatisticsImpl
{
public:
  using NeighborListType = NeighborList<T>;

  FindNeighborListStatisticsImpl(const IFilter* filter, INeighborList& source, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation,
                                 std::vector<IDataArray*>& arrays)
  : m_Filter(filter)
  , m_Source(source)
  , m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Median(median)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_Arrays(arrays)
  {
  }

  virtual ~FindNeighborListStatisticsImpl() = default;

  void compute(usize start, usize end) const
  {
    if constexpr(std::is_same_v<bool, T>)
    {
      return;
    }

    using DataArrayType = DataArray<T>;

    auto* array0 = dynamic_cast<DataArray<uint64_t>*>(m_Arrays[0]);
    if(m_Length && array0 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    auto* array1 = dynamic_cast<DataArrayType*>(m_Arrays[1]);
    if(m_Min && array1 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Min' array to needed type. Check input array selection.");
    }
    auto* array2 = dynamic_cast<DataArrayType*>(m_Arrays[2]);
    if(m_Max && array2 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Max' array to needed type. Check input array selection.");
    }
    auto* array3 = dynamic_cast<Float32Array*>(m_Arrays[3]);
    if(m_Mean && array3 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
    }
    auto* array4 = dynamic_cast<Float32Array*>(m_Arrays[4]);
    if(m_Median && array4 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    auto* array5 = dynamic_cast<Float32Array*>(m_Arrays[5]);
    if(m_StdDeviation && array5 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    auto* array6 = dynamic_cast<Float32Array*>(m_Arrays[6]);
    if(m_Summation && array6 == nullptr)
    {
      throw std::invalid_argument("FindNeighborListStatistics::compute() could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
    }

    NeighborListType& sourceList = dynamic_cast<NeighborListType&>(m_Source);

    for(usize i = start; i < end; i++)
    {
      const std::vector<T>& tmpList = sourceList[i];

      if(m_Length)
      {
        int64_t val = static_cast<int64_t>(tmpList.size());
        array0->initializeTuple(i, val);
      }
      if(m_Min)
      {
        T val = StatisticsCalculations::findMin(tmpList);
        array1->initializeTuple(i, val);
      }
      if(m_Max)
      {
        T val = StatisticsCalculations::findMax(tmpList);
        array2->initializeTuple(i, val);
      }
      if(m_Mean)
      {
        float val = StatisticsCalculations::findMean(tmpList);
        array3->initializeTuple(i, val);
      }
      if(m_Median)
      {
        float val = StatisticsCalculations::findMedian(tmpList);
        array4->initializeTuple(i, val);
      }
      if(m_StdDeviation)
      {
        float val = StatisticsCalculations::findStdDeviation(tmpList);
        array5->initializeTuple(i, val);
      }
      if(m_Summation)
      {
        float val = StatisticsCalculations::findSummation(tmpList);
        array6->initializeTuple(i, val);
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const IFilter* m_Filter = nullptr;
  INeighborList& m_Source;
  bool m_Length = false;
  bool m_Min = false;
  bool m_Max = false;
  bool m_Mean = false;
  bool m_Median = false;
  bool m_StdDeviation = false;
  bool m_Summation = false;

  std::vector<IDataArray*>& m_Arrays;
};
} // namespace

//------------------------------------------------------------------------------
OutputActions FindNeighborListStatistics::createCompatibleArrays(const DataStructure& data, const Arguments& args) const
{
  auto findLength = args.value<bool>(k_FindLength_Key);
  auto findMin = args.value<bool>(k_FindMinimum_Key);
  auto findMax = args.value<bool>(k_FindMaximum_Key);
  auto findMean = args.value<bool>(k_FindMean_Key);
  auto findMedian = args.value<bool>(k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(k_FindStandardDeviation_Key);
  auto findSummation = args.value<bool>(k_FindSummation_Key);

  auto inputArrayPath = args.value<DataPath>(k_InputArray_Key);
  auto* inputArray = data.getDataAs<INeighborList>(inputArrayPath);
  std::vector<usize> tupleDims{inputArray->getNumberOfTuples()};
  DataType dataType = inputArray->getDataType();
  const DataPath outputGroupPath = inputArrayPath.getParent();

  OutputActions actions;
  if(findLength)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Length_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMin)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Minimum_Key));
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMax)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Maximum_Key));
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMean)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Mean_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findMedian)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Median_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findStdDeviation)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_StandardDeviation_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }
  if(findSummation)
  {
    auto arrayPath = outputGroupPath.createChildPath(args.value<std::string>(k_Summation_Key));
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.appendAction(std::move(action));
  }

  return std::move(actions);
}

//------------------------------------------------------------------------------
std::string FindNeighborListStatistics::name() const
{
  return FilterTraits<FindNeighborListStatistics>::name;
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
  return "Find Neighbor List Statistics";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNeighborListStatistics::defaultTags() const
{
  return {className(), "NeighborList", "Statistics", "Analytics"};
}

//------------------------------------------------------------------------------
Parameters FindNeighborListStatistics::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "Specifies whether or not the filter creates the Length array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMinimum_Key, "Find Minimum", "Specifies whether or not the filter creates the Minimum array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMaximum_Key, "Find Maximum", "Specifies whether or not the filter creates the Maximum array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "Specifies whether or not the filter creates the Mean array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "Specifies whether or not the filter creates the Median array during calculations", true));
  params.insert(
      std::make_unique<BoolParameter>(k_FindStandardDeviation_Key, "Find Standard Deviation", "Specifies whether or not the filter creates the Standard Deviation array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "Specifies whether or not the filter creates the Summation array during calculations", true));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(
      std::make_unique<NeighborListSelectionParameter>(k_InputArray_Key, "NeighborList to Compute Statistics", "Input Data Array to compute statistics", DataPath(), complex::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_Length_Key, "Length", "Path to create the Length array during calculations", "Length"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Minimum_Key, "Minimum", "Path to create the Minimum array during calculations", "Minimum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Maximum_Key, "Maximum", "Path to create the Maximum array during calculations", "Maximum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Mean_Key, "Mean", "Path to create the Mean array during calculations", "Mean"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Median_Key, "Median", "Path to create the Median array during calculations", "Median"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_StandardDeviation_Key, "Standard Deviation", "Path to create the Standard Deviation array during calculations", "StandardDeviation"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_Summation_Key, "Summation", "Path to create the Summation array during calculations", "Summation"));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNeighborListStatistics::clone() const
{
  return std::make_unique<FindNeighborListStatistics>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNeighborListStatistics::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto findLength = args.value<bool>(k_FindLength_Key);
  auto findMin = args.value<bool>(k_FindMinimum_Key);
  auto findMax = args.value<bool>(k_FindMaximum_Key);
  auto findMean = args.value<bool>(k_FindMean_Key);
  auto findMedian = args.value<bool>(k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(k_FindStandardDeviation_Key);
  auto findSummation = args.value<bool>(k_FindSummation_Key);

  auto inputArrayPath = args.value<DataPath>(k_InputArray_Key);

  //
  if(!findMin && !findMax && !findMean && !findMedian && !findStdDeviation && !findSummation && !findLength)
  {
    std::string ss = fmt::format("No statistics have been selected");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_NoAction, ss}})};
  }

  std::vector<DataPath> dataArrayPaths;

  auto inputArray = data.getDataAs<INeighborList>(inputArrayPath);
  if(inputArray == nullptr)
  {
    std::string ss = fmt::format("Missing input array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, ss}})};
  }

  dataArrayPaths.push_back(inputArrayPath);

  return {std::move(createCompatibleArrays(data, args))};
}

//------------------------------------------------------------------------------
Result<> FindNeighborListStatistics::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto findLength = args.value<bool>(k_FindLength_Key);
  auto findMin = args.value<bool>(k_FindMinimum_Key);
  auto findMax = args.value<bool>(k_FindMaximum_Key);
  auto findMean = args.value<bool>(k_FindMean_Key);
  auto findMedian = args.value<bool>(k_FindMedian_Key);
  auto findStdDeviation = args.value<bool>(k_FindStandardDeviation_Key);
  auto findSummation = args.value<bool>(k_FindSummation_Key);

  if(!findMin && !findMax && !findMean && !findMedian && !findStdDeviation && findSummation && !findLength)
  {
    return {};
  }

  auto inputArrayPath = args.value<DataPath>(k_InputArray_Key);
  auto& inputArray = data.getDataRefAs<INeighborList>(inputArrayPath);
  const DataPath outputGroupPath = inputArrayPath.getParent();

  std::vector<IDataArray*> arrays(7, nullptr);

  if(findLength)
  {
    auto lengthPath = outputGroupPath.createChildPath(args.value<std::string>(k_Length_Key));
    arrays[0] = data.getDataAs<IDataArray>(lengthPath);
  }
  if(findMin)
  {
    auto minPath = outputGroupPath.createChildPath(args.value<std::string>(k_Minimum_Key));
    arrays[1] = data.getDataAs<IDataArray>(minPath);
  }
  if(findMax)
  {
    auto maxPath = outputGroupPath.createChildPath(args.value<std::string>(k_Maximum_Key));
    arrays[2] = data.getDataAs<IDataArray>(maxPath);
  }
  if(findMean)
  {
    auto meanPath = outputGroupPath.createChildPath(args.value<std::string>(k_Mean_Key));
    arrays[3] = data.getDataAs<IDataArray>(meanPath);
  }
  if(findMedian)
  {
    auto medianPath = outputGroupPath.createChildPath(args.value<std::string>(k_Median_Key));
    arrays[4] = data.getDataAs<IDataArray>(medianPath);
  }
  if(findStdDeviation)
  {
    auto stdDeviationPath = outputGroupPath.createChildPath(args.value<std::string>(k_StandardDeviation_Key));
    arrays[5] = data.getDataAs<IDataArray>(stdDeviationPath);
  }
  if(findSummation)
  {
    auto summationPath = outputGroupPath.createChildPath(args.value<std::string>(k_Summation_Key));
    arrays[6] = data.getDataAs<IDataArray>(summationPath);
  }

  DataType type = inputArray.getDataType();
  if(type == DataType::boolean)
  {
    std::string ss = fmt::format("FindNeighborListStatistics::NeighborList {} was of type boolean, and thus cannot be processed", inputArray.getName());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_BoolTypeNeighborList, ss}})};
  }

  usize numTuples = inputArray.getNumberOfTuples();
  if(numTuples == 0)
  {
    std::string ss = fmt::format("FindNeighborListStatistics::NeighborList {} was empty", inputArray.getName());
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_EmptyNeighborList, ss}})};
  }

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numTuples);
  ExecuteParallelFunction<FindNeighborListStatisticsImpl, NoBooleanType>(type, dataAlg, this, inputArray, findLength, findMin, findMax, findMean, findMedian, findStdDeviation, findSummation, arrays);

  return {};
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

Result<Arguments> FindNeighborListStatistics::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindNeighborListStatistics().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindLengthKey, k_FindLength_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMinKey, k_FindMinimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMaxKey, k_FindMaximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMeanKey, k_FindMean_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindMedianKey, k_FindMedian_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindStdDeviationKey, k_FindStandardDeviation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_FindSummationKey, k_FindSummation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_InputArray_Key));
  // results.push_back(
  //    SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_DestinationAttributeMatrixKey, "@COMPLEX_PARAMETER_KEY@"));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_LengthArrayNameKey, k_Length_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MinimumArrayNameKey, k_Minimum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MaximumArrayNameKey, k_Maximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MeanArrayNameKey, k_Mean_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MedianArrayNameKey, k_Median_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_StdDeviationArrayNameKey, k_StandardDeviation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_SummationArrayNameKey, k_Summation_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex

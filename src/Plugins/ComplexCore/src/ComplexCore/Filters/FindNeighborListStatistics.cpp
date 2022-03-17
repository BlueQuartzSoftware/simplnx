#include "FindNeighborListStatistics.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Utilities/Math/StatisticsCalculations.hpp"

namespace complex
{
namespace
{
constexpr int64 k_NoAction = -6800;
constexpr int64 k_MissingInputArray = -6801;
constexpr int64 k_WrongInputArrayType = -6802;
constexpr int64 k_NonScalarInputArray = -6803;

template <typename T>
class FindNeighborListStatisticsImpl
{
public:
  using NeighborListType = NeighborList<T>;

  FindNeighborListStatisticsImpl(const IFilter* filter, NeighborListType& source, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation,
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
    for(usize i = start; i < end; i++)
    {
      std::vector<T>& tmpList = m_Source[i];

      if(m_Length)
      {
        if(auto* array = dynamic_cast<DataArray<usize>*>(m_Arrays[0]))
        {
          int64_t val = static_cast<int64_t>(tmpList.size());
          array->initializeTuple(i, val);
        }
      }
      if(m_Min)
      {
        if(auto* array = dynamic_cast<DataArray<T>*>(m_Arrays[1]))
        {
          T val = StaticicsCalculations::findMin(tmpList);
          array->initializeTuple(i, val);
        }
      }
      if(m_Max)
      {
        if(auto* array = dynamic_cast<DataArray<T>*>(m_Arrays[2]))
        {
          T val = StaticicsCalculations::findMax(tmpList);
          array->initializeTuple(i, val);
        }
      }
      if(m_Mean)
      {
        if(auto* array = dynamic_cast<Float32Array*>(m_Arrays[3]))
        {
          float val = StaticicsCalculations::findMean(tmpList);
          array->initializeTuple(i, val);
        }
      }
      if(m_Median)
      {
        if(auto* array = dynamic_cast<Float32Array*>(m_Arrays[4]))
        {
          float val = StaticicsCalculations::findMedian(tmpList);
          array->initializeTuple(i, val);
        }
      }
      if(m_StdDeviation)
      {
        if(auto* array = dynamic_cast<Float32Array*>(m_Arrays[5]))
        {
          float val = StaticicsCalculations::findStdDeviation(tmpList);
          array->initializeTuple(i, val);
        }
      }
      if(m_Summation)
      {
        if(auto* array = dynamic_cast<Float32Array*>(m_Arrays[6]))
        {
          float val = StaticicsCalculations::findSummation(tmpList);
          array->initializeTuple(i, val);
        }
      }
    }
  }

#if 0
  void operator()(const SIMPLRange& range) const
  {
    compute(range.min(), range.max());
  }
#endif

private:
  const IFilter* m_Filter = nullptr;
  NeighborListType& m_Source;
  bool m_Length = false;
  bool m_Min = false;
  bool m_Max = false;
  bool m_Mean = false;
  bool m_Median = false;
  bool m_StdDeviation = false;
  bool m_Summation = false;

  std::vector<IDataArray*>& m_Arrays;
};

template <typename DataType>
void findStatisticsImpl(const IFilter* filter, INeighborList& source, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation, std::vector<IDataArray*>& arrays)
{
  usize numTuples = source.getNumberOfTuples();
  auto sourceList = dynamic_cast<NeighborList<DataType>&>(source);
  FindNeighborListStatisticsImpl<DataType> algorithm(filter, sourceList, length, min, max, mean, median, stdDeviation, summation, arrays);
  algorithm.compute(0, numTuples - 1);

#if 0
  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numTuples);
  dataAlg.execute(FindNeighborListStatisticsImpl<T>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays));
#endif
}

void findStatistics(const IFilter* filter, INeighborList& source, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation, std::vector<IDataArray*>& arrays)
{
  usize numTuples = source.getNumberOfTuples();
  if(numTuples == 0)
  {
    return;
  }

  auto dataType = source.getDataType();
  switch(dataType)
  {
  case DataType::int8:
    findStatisticsImpl<int8>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::int16:
    findStatisticsImpl<int16>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::int32:
    findStatisticsImpl<int32>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::int64:
    findStatisticsImpl<int64>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::uint8:
    findStatisticsImpl<uint8>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::uint16:
    findStatisticsImpl<uint16>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::uint32:
    findStatisticsImpl<uint32>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::uint64:
    findStatisticsImpl<uint64>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::float32:
    findStatisticsImpl<float32>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::float64:
    findStatisticsImpl<float64>(filter, source, length, min, max, mean, median, stdDeviation, summation, arrays);
    return;
  case DataType::boolean:
    return;
  }
}
} // namespace

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

  OutputActions actions;
  if(findLength)
  {
    auto arrayPath = args.value<DataPath>(k_Length_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findMin)
  {
    auto arrayPath = args.value<DataPath>(k_Minimum_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findMax)
  {
    auto arrayPath = args.value<DataPath>(k_Maximum_Key);
    auto action = std::make_unique<CreateArrayAction>(dataType, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findMean)
  {
    auto arrayPath = args.value<DataPath>(k_Mean_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findMedian)
  {
    auto arrayPath = args.value<DataPath>(k_Median_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findStdDeviation)
  {
    auto arrayPath = args.value<DataPath>(k_StandardDeviation_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }
  if(findSummation)
  {
    auto arrayPath = args.value<DataPath>(k_Summation_Key);
    auto action = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, std::vector<usize>{1}, arrayPath);
    actions.actions.push_back(std::move(action));
  }

  return std::move(actions);
}

std::string FindNeighborListStatistics::name() const
{
  return FilterTraits<FindNeighborListStatistics>::name;
}

std::string FindNeighborListStatistics::className() const
{
  return FilterTraits<FindNeighborListStatistics>::className;
}

Uuid FindNeighborListStatistics::uuid() const
{
  return FilterTraits<FindNeighborListStatistics>::uuid;
}

std::string FindNeighborListStatistics::humanName() const
{
  return "Find Neighbor List Statistics";
}

Parameters FindNeighborListStatistics::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "Specifies whether or not the filter creates the Length array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMinimum_Key, "Find Minimum", "Specifies whether or not the filter creates the Minimum array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMaximum_Key, "Find Maximum", "Specifies whether or not the filter creates the Maximum array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "Specifies whether or not the filter creates the Mean array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindMedian_Key, "Find Median", "Specifies whether or not the filter creates the Median array during calculations", true));
  params.insert(
      std::make_unique<BoolParameter>(k_FindStandardDeviation_Key, "Find Standard Deviation", "Specifies whether or not the filter creates the Standard Deviation array during calculations", true));
  params.insert(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "Specifies whether or not the filter creates the Summation array during calculations", true));

  params.insert(
      std::make_unique<ArraySelectionParameter>(k_InputArray_Key, "Arrays to Compute Statistics", "Specifies whether or not the filter creates the Summation array during calculations", DataPath()));

  params.insert(std::make_unique<ArrayCreationParameter>(k_Length_Key, "Length", "Path to create the Length array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Minimum_Key, "Minimum", "Path to create the Minimum array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Maximum_Key, "Maximum", "Path to create the Maximum array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Mean_Key, "Mean", "Path to create the Mean array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Median_Key, "Median", "Path to create the Mean array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_StandardDeviation_Key, "Standard Deviation", "Path to create the Standard Deviation array during calculations", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Summation_Key, "Summation", "Path to create the Summation array during calculations", DataPath()));
  return params;
}

IFilter::UniquePointer FindNeighborListStatistics::clone() const
{
  return std::make_unique<FindNeighborListStatistics>();
}

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
  // The input array must be a NeighborList.
  if(inputArray->getTypeName() != "NeighborList<T>")
  {
    std::string ss = fmt::format("Input Data must be a NeighborList Attribute Array");
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_WrongInputArrayType, ss}})};
  }
  dataArrayPaths.push_back(inputArrayPath);

  return {std::move(createCompatibleArrays(data, args))};
}

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
  auto* inputArray = data.getDataAs<INeighborList>(inputArrayPath);

  std::vector<IDataArray*> arrays(7, nullptr);

  if(findLength)
  {
    auto lengthPath = args.value<DataPath>(k_Length_Key);
    arrays[0] = data.getDataAs<IDataArray>(lengthPath);
  }
  if(findMin)
  {
    auto minPath = args.value<DataPath>(k_Minimum_Key);
    arrays[1] = data.getDataAs<IDataArray>(minPath);
  }
  if(findMax)
  {
    auto maxPath = args.value<DataPath>(k_Maximum_Key);
    arrays[2] = data.getDataAs<IDataArray>(maxPath);
  }
  if(findMean)
  {
    auto meanPath = args.value<DataPath>(k_Mean_Key);
    arrays[3] = data.getDataAs<IDataArray>(meanPath);
  }
  if(findMedian)
  {
    auto medianPath = args.value<DataPath>(k_Median_Key);
    arrays[4] = data.getDataAs<IDataArray>(medianPath);
  }
  if(findStdDeviation)
  {
    auto stdDeviationPath = args.value<DataPath>(k_StandardDeviation_Key);
    arrays[5] = data.getDataAs<IDataArray>(stdDeviationPath);
  }
  if(findSummation)
  {
    auto summationPath = args.value<DataPath>(k_Summation_Key);
    arrays[6] = data.getDataAs<IDataArray>(summationPath);
  }

  findStatistics(this, *inputArray, findLength, findMin, findMax, findMean, findMedian, findStdDeviation, findSummation, arrays);

  return {};
}
} // namespace complex

#include "SimplnxCore/Filters/Algorithms/ComputeArrayStatistics.hpp"
#include "SimplnxCore/Filters/ComputeArrayStatisticsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <optional>
#include <system_error>
#include <vector>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/global_control.h>
#endif

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
std::optional<UnitTest::AlgorithmTestScenario> SelectScanlineInMemoryScenario()
{
  const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
  const auto iter = std::find(scenarios.cbegin(), scenarios.cend(), UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  if(iter == scenarios.cend())
  {
    return std::nullopt;
  }
  return *iter;
}

template <typename T>
class StatisticsFailingReadDataStore : public DataStore<T>
{
public:
  StatisticsFailingReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(m_ErrorCode, "Injected statistics bulk-read failure");
  }

private:
  int32 m_ErrorCode = 0;
};

template <typename T>
class StatisticsFailingWriteDataStore : public DataStore<T>
{
public:
  StatisticsFailingWriteDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected statistics bulk-write failure");
  }

private:
  int32 m_ErrorCode = 0;
};

template <typename T>
class StatisticsCancelAfterReadDataStore : public DataStore<T>
{
public:
  StatisticsCancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && !m_DidCancel)
    {
      m_DidCancel = true;
      m_ShouldCancel.store(true);
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable bool m_DidCancel = false;
};

/**
 * @class StatisticsCancelAfterReadsDataStore
 * @brief Sets the cancellation flag after a chosen number of successful bulk reads, and counts every bulk read.
 */
template <typename T>
class StatisticsCancelAfterReadsDataStore : public DataStore<T>
{
public:
  StatisticsCancelAfterReadsDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel, usize cancelAfterReads)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  , m_CancelAfterReads(cancelAfterReads)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    ++m_ReadCount;
    if(result.valid() && m_ReadCount == m_CancelAfterReads)
    {
      m_ShouldCancel.store(true);
    }
    return result;
  }

  usize readCount() const
  {
    return m_ReadCount;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  usize m_CancelAfterReads = 0;
  mutable usize m_ReadCount = 0;
};

template <typename T>
class StatisticsCancelAfterWriteDataStore : public DataStore<T>
{
public:
  StatisticsCancelAfterWriteDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    Result<> result = DataStore<T>::copyFromBuffer(startIndex, buffer);
    if(result.valid() && !m_DidCancel)
    {
      m_DidCancel = true;
      m_ShouldCancel.store(true);
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  bool m_DidCancel = false;
};

ComputeArrayStatisticsInputValues CreateMinimalStatisticsInputValues(const DataPath& inputPath, const DataPath& destinationPath)
{
  ComputeArrayStatisticsInputValues inputValues{};
  inputValues.RangeType = to_underlying(ComputeArrayStatistics::FeatureIdRangeControls::None);
  inputValues.Range = {0, -1};
  inputValues.SelectedArrayPath = inputPath;
  inputValues.DestinationAttributeMatrix = destinationPath;
  inputValues.FeatureHasDataArrayName = destinationPath.createChildPath("FeatureHasData");
  inputValues.LengthArrayName = destinationPath.createChildPath("Length");
  inputValues.MinimumArrayName = destinationPath.createChildPath("Minimum");
  inputValues.MaximumArrayName = destinationPath.createChildPath("Maximum");
  inputValues.MeanArrayName = destinationPath.createChildPath("Mean");
  inputValues.MedianArrayName = destinationPath.createChildPath("Median");
  inputValues.ModeArrayName = destinationPath.createChildPath("Mode");
  inputValues.StdDeviationArrayName = destinationPath.createChildPath("StandardDeviation");
  inputValues.SummationArrayName = destinationPath.createChildPath("Summation");
  inputValues.StandardizedArrayName = inputPath.replaceName("Standardized");
  inputValues.NumUniqueValuesName = destinationPath.createChildPath("NumUniqueValues");
  inputValues.FeatureIdMapArrayPath = destinationPath.createChildPath("FeatureIdMap");
  return inputValues;
}

/**
 * @class ScanSeamScope
 * @brief Applies small page and block sizes to the whole-array statistics engines and restores the production sizes.
 *
 * Small sizes let a test array of a few thousand tuples span many pages, end in a partial page, and hold several
 * blocks in each page. The constructor also clears the parallel dispatch witness.
 */
class ScanSeamScope
{
public:
  ScanSeamScope(usize pageBytes, usize blockTuples)
  {
    SetComputeArrayStatisticsPageBytesForTesting(pageBytes);
    SetComputeArrayStatisticsBlockTuplesForTesting(blockTuples);
    ResetComputeArrayStatisticsScanWitnessForTesting();
  }

  ~ScanSeamScope()
  {
    SetComputeArrayStatisticsPageBytesForTesting(0);
    SetComputeArrayStatisticsBlockTuplesForTesting(0);
    ResetComputeArrayStatisticsScanWitnessForTesting();
  }

  ScanSeamScope(const ScanSeamScope&) = delete;
  ScanSeamScope(ScanSeamScope&&) = delete;
  ScanSeamScope& operator=(const ScanSeamScope&) = delete;
  ScanSeamScope& operator=(ScanSeamScope&&) = delete;
};

constexpr usize k_SeamPageBytes = 1024; // Page budget for the small-array engine tests.
constexpr usize k_SeamBlockTuples = 64; // Block size for the small-array engine tests.

template <typename T>
bool VectorContains(const std::vector<T>& vector, T value)
{
  return (std::find(vector.begin(), vector.end(), value) != vector.end());
}

Arguments CreateAllStatisticsArguments(const DataPath& inputPath, const DataPath& destinationPath, const std::optional<DataPath>& featureIdsPath = {}, const std::optional<DataPath>& maskPath = {},
                                       ChoicesParameter::ValueType rangeType = 0, std::vector<int32> range = {0, -1})
{
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(destinationPath));
  if(featureIdsPath.has_value())
  {
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(*featureIdsPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(rangeType));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<std::vector<int32>>(std::move(range)));
  }
  if(maskPath.has_value())
  {
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(*maskPath));
  }
  return args;
}

template <typename T>
void RunCompactNumericStatistics(UnitTest::AlgorithmTestScope& scope)
{
  DataStructure dataStructure;
  auto* values = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, "Values", {4}, {1});
  REQUIRE(values != nullptr);
  const std::array<T, 4> input = {static_cast<T>(1), static_cast<T>(2), static_cast<T>(2), static_cast<T>(5)};
  auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  {
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
  }
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 4);
  REQUIRE(dataStructure.getDataRefAs<DataArray<T>>(DataPath({"Statistics", "Minimum"}))[0] == static_cast<T>(1));
  REQUIRE(dataStructure.getDataRefAs<DataArray<T>>(DataPath({"Statistics", "Maximum"}))[0] == static_cast<T>(5));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Summation"}))[0] == Approx(10.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Mean"}))[0] == Approx(2.5F));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == Approx(2.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "StandardDeviation"}))[0] == Approx(1.5F));
  REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 3);
  if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  {
    REQUIRE(dataStructure.getDataRefAs<NeighborList<T>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<T>{static_cast<T>(2)});
  }
}

template <>
void RunCompactNumericStatistics<bool>(UnitTest::AlgorithmTestScope& scope)
{
  DataStructure dataStructure;
  auto* values = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Values", {4}, {1});
  REQUIRE(values != nullptr);
  const std::array<bool, 4> input = {false, true, true, true};
  auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 4);
  REQUIRE_FALSE(dataStructure.getDataRefAs<BoolArray>(DataPath({"Statistics", "Minimum"}))[0]);
  REQUIRE(dataStructure.getDataRefAs<BoolArray>(DataPath({"Statistics", "Maximum"}))[0]);
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Summation"}))[0] == 3.0F);
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Mean"}))[0] == 0.75F);
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == 1.0F);
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "StandardDeviation"}))[0] == Approx(0.4330127F));
  REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 2);
}

struct StatisticsParityResult
{
  std::vector<uint64> Length;
  std::vector<int32> Minimum;
  std::vector<int32> Maximum;
  std::vector<float32> Mean;
  std::vector<float32> Median;
  std::vector<float32> StandardDeviation;
  std::vector<float32> Summation;
  std::vector<int32> Unique;
  std::vector<uint8> FeatureHasData;
  std::vector<std::vector<int32>> Modes;
  std::vector<float32> Standardized;
};

StatisticsParityResult RunStatisticsParityCase(UnitTest::AlgorithmTestScope& scope, bool computeByIndex, int32 maskKind)
{
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroup != nullptr);
  constexpr usize k_TupleCount = 8;
  auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {k_TupleCount}, {1}, inputGroup->getId());
  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {k_TupleCount}, {1}, inputGroup->getId());
  REQUIRE(values != nullptr);
  REQUIRE(featureIds != nullptr);
  const std::array<int32, k_TupleCount> input = {4, 1, 4, 2, 8, 2, 6, 1};
  const std::array<int32, k_TupleCount> ids = {0, 0, 1, 1, 2, 2, 4, 4};
  const std::array<bool, k_TupleCount> boolMaskValues = {true, true, true, false, true, true, false, true};
  const std::array<uint8, k_TupleCount> uint8MaskValues = {1, 1, 1, 0, 1, 1, 0, 1};
  auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  auto featureIdsWriteResult = featureIds->getDataStoreRef().copyFromBuffer(0, ids);
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
  std::optional<DataPath> maskPath;
  if(maskKind == 1)
  {
    auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
    REQUIRE(mask != nullptr);
    auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, boolMaskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    maskPath = DataPath({"Input", "Mask"});
  }
  else if(maskKind == 2)
  {
    auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
    REQUIRE(mask != nullptr);
    auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, uint8MaskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    maskPath = DataPath({"Input", "Mask"});
  }

  ComputeArrayStatisticsFilter filter;
  Arguments args =
      CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), computeByIndex ? std::optional<DataPath>{DataPath({"Input", "FeatureIds"})} : std::nullopt, maskPath);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath statisticsPath({"Statistics"});
  const usize groupCount = dataStructure.getDataRefAs<UInt64Array>(statisticsPath.createChildPath("Length")).getNumberOfTuples();
  StatisticsParityResult result;
  result.Length.resize(groupCount);
  result.Minimum.resize(groupCount);
  result.Maximum.resize(groupCount);
  result.Mean.resize(groupCount);
  result.Median.resize(groupCount);
  result.StandardDeviation.resize(groupCount);
  result.Summation.resize(groupCount);
  result.Unique.resize(groupCount);
  result.Modes.resize(groupCount);
  result.Standardized.resize(k_TupleCount);
  auto lengthReadResult = dataStructure.getDataRefAs<UInt64Array>(statisticsPath.createChildPath("Length")).getDataStoreRef().copyIntoBuffer(0, result.Length);
  SIMPLNX_RESULT_REQUIRE_VALID(lengthReadResult);
  auto minimumReadResult = dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("Minimum")).getDataStoreRef().copyIntoBuffer(0, result.Minimum);
  SIMPLNX_RESULT_REQUIRE_VALID(minimumReadResult);
  auto maximumReadResult = dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("Maximum")).getDataStoreRef().copyIntoBuffer(0, result.Maximum);
  SIMPLNX_RESULT_REQUIRE_VALID(maximumReadResult);
  auto meanReadResult = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Mean")).getDataStoreRef().copyIntoBuffer(0, result.Mean);
  SIMPLNX_RESULT_REQUIRE_VALID(meanReadResult);
  auto medianReadResult = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Median")).getDataStoreRef().copyIntoBuffer(0, result.Median);
  SIMPLNX_RESULT_REQUIRE_VALID(medianReadResult);
  auto standardDeviationReadResult = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("StandardDeviation")).getDataStoreRef().copyIntoBuffer(0, result.StandardDeviation);
  SIMPLNX_RESULT_REQUIRE_VALID(standardDeviationReadResult);
  auto summationReadResult = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Summation")).getDataStoreRef().copyIntoBuffer(0, result.Summation);
  SIMPLNX_RESULT_REQUIRE_VALID(summationReadResult);
  auto numUniqueValuesReadResult = dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("NumUniqueValues")).getDataStoreRef().copyIntoBuffer(0, result.Unique);
  SIMPLNX_RESULT_REQUIRE_VALID(numUniqueValuesReadResult);
  auto standardizedReadResult = dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, result.Standardized);
  SIMPLNX_RESULT_REQUIRE_VALID(standardizedReadResult);
  if(computeByIndex)
  {
    result.FeatureHasData.resize(groupCount);
    auto featureHasData = std::make_unique<bool[]>(groupCount);
    auto featureHasDataReadResult =
        dataStructure.getDataRefAs<BoolArray>(statisticsPath.createChildPath("FeatureHasData")).getDataStoreRef().copyIntoBuffer(0, nonstd::span<bool>(featureHasData.get(), groupCount));
    SIMPLNX_RESULT_REQUIRE_VALID(featureHasDataReadResult);
    for(usize group = 0; group < groupCount; ++group)
    {
      result.FeatureHasData[group] = featureHasData[group] ? 1 : 0;
    }
  }
  const auto& mode = dataStructure.getDataRefAs<NeighborList<int32>>(statisticsPath.createChildPath("Mode"));
  for(usize group = 0; group < groupCount; ++group)
  {
    result.Modes[group] = mode.getList(static_cast<int32>(group));
  }
  return result;
}
#if SIMPLNX_TEST_ALGORITHM_PATH == 1
class FailingStatisticsExternalSort : public IExternalSort
{
public:
  explicit FailingStatisticsExternalSort(int32 errorCode)
  : m_ErrorCode(errorCode)
  {
  }

  Result<> append(uint64, nonstd::span<const std::byte>, const std::atomic_bool&, const ExternalSortProgressCallback&) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected statistics external-sort append failure");
  }

  Result<> finish(const std::atomic_bool&, const ExternalSortProgressCallback&) override
  {
    return {};
  }

  Result<uint64> read(uint64, uint64, nonstd::span<std::byte>, const std::atomic_bool&) const override
  {
    return {uint64{0}};
  }

  uint64 recordCount() const override
  {
    return 0;
  }

private:
  int32 m_ErrorCode = 0;
};

class FailingStatisticsExternalSortManager : public IDataIOManager
{
public:
  FailingStatisticsExternalSortManager(std::string format, int32 errorCode)
  : m_Format(std::move(format))
  , m_ErrorCode(errorCode)
  {
  }

  std::string formatName() const override
  {
    return m_Format;
  }

  bool supportsExternalSort() const override
  {
    return true;
  }

  Result<std::unique_ptr<IExternalSort>> createExternalSort(const ExternalSortConfig&) const override
  {
    return {std::make_unique<FailingStatisticsExternalSort>(m_ErrorCode)};
  }

private:
  std::string m_Format;
  int32 m_ErrorCode = 0;
};

class StatisticsIOManagerRestore
{
public:
  StatisticsIOManagerRestore(DataIOCollection& collection, std::shared_ptr<IDataIOManager> manager)
  : m_Collection(collection)
  , m_Manager(std::move(manager))
  {
  }

  ~StatisticsIOManagerRestore()
  {
    if(m_Manager != nullptr)
    {
      (void)m_Collection.addIOManager(m_Manager);
    }
  }

private:
  DataIOCollection& m_Collection;
  std::shared_ptr<IDataIOManager> m_Manager;
};

#endif
} // namespace

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Direct and Scanline mask grouping parity", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const bool computeByIndex = GENERATE(false, true);
  const int32 maskKind = GENERATE(0, 1, 2);
  CAPTURE(computeByIndex, maskKind);
  StatisticsParityResult expected;
  bool hasExpectedResult = false;
  const auto requireFloatParity = [](const std::vector<float32>& actual, const std::vector<float32>& expectedValues) {
    REQUIRE(actual.size() == expectedValues.size());
    for(usize index = 0; index < actual.size(); ++index)
    {
      if(std::isnan(expectedValues[index]))
      {
        REQUIRE(std::isnan(actual[index]));
      }
      else
      {
        REQUIRE(actual[index] == Approx(expectedValues[index]).margin(1.0E-5F));
      }
    }
  };
  const auto scenarios = UnitTest::SelectAlgorithmTestScenariosForInMemoryStores();
  for(const auto scenario : scenarios)
  {
    CAPTURE(scenario);
    UnitTest::AlgorithmTestScope scope(scenario);
    const auto actual = RunStatisticsParityCase(scope, computeByIndex, maskKind);
    // Reduce the eight known input values independently. This reference uses
    // sorted small vectors and population variance, without production helpers.
    const std::array<int32, 8> input = {4, 1, 4, 2, 8, 2, 6, 1};
    const std::array<usize, 8> groups = {0, 0, 1, 1, 2, 2, 4, 4};
    const usize groupCount = computeByIndex ? 5 : 1;
    REQUIRE(actual.Length.size() == groupCount);
    REQUIRE(actual.Minimum.size() == groupCount);
    REQUIRE(actual.Maximum.size() == groupCount);
    REQUIRE(actual.Mean.size() == groupCount);
    REQUIRE(actual.Median.size() == groupCount);
    REQUIRE(actual.StandardDeviation.size() == groupCount);
    REQUIRE(actual.Summation.size() == groupCount);
    REQUIRE(actual.Unique.size() == groupCount);
    REQUIRE(actual.Modes.size() == groupCount);
    REQUIRE(actual.Standardized.size() == input.size());
    for(usize groupIdx = 0; groupIdx < groupCount; ++groupIdx)
    {
      CAPTURE(groupIdx);
      std::vector<int32> selected;
      for(usize tupleIdx = 0; tupleIdx < input.size(); ++tupleIdx)
      {
        if((maskKind == 0 || (tupleIdx != 3 && tupleIdx != 6)) && (!computeByIndex || groups[tupleIdx] == groupIdx))
        {
          selected.push_back(input[tupleIdx]);
        }
      }
      CHECK(actual.Length[groupIdx] == selected.size());
      if(computeByIndex)
      {
        REQUIRE(actual.FeatureHasData.size() == groupCount);
        CHECK(actual.FeatureHasData[groupIdx] == (selected.empty() ? 0 : 1));
      }
      if(selected.empty())
      {
        CHECK(actual.Minimum[groupIdx] == std::numeric_limits<int32>::max());
        CHECK(actual.Maximum[groupIdx] == std::numeric_limits<int32>::min());
        CHECK(actual.Mean[groupIdx] == 0.0F);
        CHECK(actual.Median[groupIdx] == 0.0F);
        CHECK(actual.StandardDeviation[groupIdx] == 0.0F);
        CHECK(actual.Summation[groupIdx] == 0.0F);
        CHECK(actual.Unique[groupIdx] == 0);
        CHECK(actual.Modes[groupIdx].empty());
        continue;
      }
      std::sort(selected.begin(), selected.end());
      float64 sum = 0.0;
      for(const int32 value : selected)
      {
        sum += value;
      }
      const float64 mean = sum / static_cast<float64>(selected.size());
      float64 variance = 0.0;
      for(const int32 value : selected)
      {
        variance += (value - mean) * (value - mean);
      }
      const float64 deviation = std::sqrt(variance / static_cast<float64>(selected.size()));
      CHECK(actual.Minimum[groupIdx] == selected.front());
      CHECK(actual.Maximum[groupIdx] == selected.back());
      CHECK(actual.Summation[groupIdx] == Approx(sum));
      CHECK(actual.Mean[groupIdx] == Approx(mean));
      CHECK(actual.StandardDeviation[groupIdx] == Approx(deviation).margin(1.0E-5));
      CHECK(actual.Median[groupIdx] == Approx((selected[(selected.size() - 1) / 2] + selected[selected.size() / 2]) / 2.0));
      std::vector<int32> modes;
      usize maxFrequency = 0;
      int32 uniqueCount = 0;
      for(usize index = 0; index < selected.size();)
      {
        const usize next = static_cast<usize>(std::upper_bound(selected.begin() + index, selected.end(), selected[index]) - selected.begin());
        const usize frequency = next - index;
        if(frequency > maxFrequency)
        {
          modes.clear();
          maxFrequency = frequency;
        }
        if(frequency == maxFrequency)
        {
          modes.push_back(selected[index]);
        }
        ++uniqueCount;
        index = next;
      }
      CHECK(actual.Unique[groupIdx] == uniqueCount);
      CHECK(actual.Modes[groupIdx] == modes);
      for(usize tupleIdx = 0; tupleIdx < input.size(); ++tupleIdx)
      {
        if(computeByIndex && groups[tupleIdx] != groupIdx)
        {
          continue;
        }
        if(maskKind != 0 && (tupleIdx == 3 || tupleIdx == 6))
        {
          CHECK(actual.Standardized[tupleIdx] == 0.0F);
        }
        else if(deviation == 0.0)
        {
          CHECK(std::isnan(actual.Standardized[tupleIdx]));
        }
        else
        {
          CHECK(actual.Standardized[tupleIdx] == Approx((input[tupleIdx] - mean) / deviation).margin(1.0E-5));
        }
      }
    }
    if(hasExpectedResult)
    {
      REQUIRE(actual.Length == expected.Length);
      REQUIRE(actual.Minimum == expected.Minimum);
      REQUIRE(actual.Maximum == expected.Maximum);
      REQUIRE(actual.Unique == expected.Unique);
      REQUIRE(actual.FeatureHasData == expected.FeatureHasData);
      REQUIRE(actual.Modes == expected.Modes);
      requireFloatParity(actual.Mean, expected.Mean);
      requireFloatParity(actual.Median, expected.Median);
      requireFloatParity(actual.StandardDeviation, expected.StandardDeviation);
      requireFloatParity(actual.Summation, expected.Summation);
      requireFloatParity(actual.Standardized, expected.Standardized);
    }
    else
    {
      expected = actual;
      hasExpectedResult = true;
    }
  }
}
TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Scanline propagates bulk store failures", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  constexpr int32 k_ReadError = -923901;
  constexpr int32 k_WriteError = -923902;

  SECTION("input bulk read")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto inputStore = std::make_shared<StatisticsFailingReadDataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1}, k_ReadError);
    auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
    auto* length = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, output->getId());
    REQUIRE(values != nullptr);
    REQUIRE(length != nullptr);

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    const std::atomic_bool shouldCancel = false;
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_ReadError);
  }

  SECTION("numeric output bulk write")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroup->getId());
    auto lengthStore = std::make_shared<StatisticsFailingWriteDataStore<uint64>>(ShapeType{1}, ShapeType{1}, std::optional<uint64>{0}, k_WriteError);
    auto* length = UInt64Array::Create(dataStructure, "Length", lengthStore, output->getId());
    REQUIRE(values != nullptr);
    REQUIRE(length != nullptr);
    const std::array<int32, 4> input = {1, 2, 3, 4};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    const std::atomic_bool shouldCancel = false;
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: a damaged out-of-core backing file surfaces as a filter error", "[SimplnxCore][ComputeArrayStatisticsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  auto& ioCollection = DataStoreUtilities::GetIOCollection();
  if(!ioCollection.hasDataStoreCreationFunction("HDF5-OOC"))
  {
    return;
  }

  UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_TupleCount = (64ULL * 1024ULL * 1024ULL) / sizeof(int32);
  const DataPath inputPath({"DamagedInput"});
  const DataPath statisticsPath({"Statistics"});

  DataStructure dataStructure;
  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, inputPath, {k_TupleCount}, {1});
  REQUIRE(inputStore != nullptr);
  auto* inputArray = Int32Array::Create(dataStructure, inputPath.getTargetName(), inputStore);
  REQUIRE(inputArray != nullptr);
  REQUIRE(inputArray->getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);

  const auto recoveryMetadata = inputArray->getDataStoreRef().getRecoveryMetadata();
  const auto backingPathIter = recoveryMetadata.find("OocBackingFilePath");
  REQUIRE(backingPathIter != recoveryMetadata.cend());
  const fs::path backingPath = backingPathIter->second;
  REQUIRE(fs::exists(backingPath));

  ioCollection.finalizeStores(dataStructure);
  ioCollection.shutdownManagers();
  std::error_code resizeError;
  fs::resize_file(backingPath, 1024, resizeError);
  INFO("Could not truncate the out-of-core backing file '" << backingPath.string() << "': " << resizeError.message());
  REQUIRE_FALSE(resizeError);

  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statisticsPath));

  const auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
  REQUIRE_FALSE(executeResult.result.errors().empty());
  const std::string expectedMessage = "bulk read failed for array '" + inputPath.toString() + "'";
  const bool identifiesBulkReadFailure = std::any_of(executeResult.result.errors().cbegin(), executeResult.result.errors().cend(),
                                                     [&expectedMessage](const Error& error) { return error.code == -6032 && error.message.find(expectedMessage) != std::string::npos; });
  REQUIRE(identifiesBulkReadFailure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: entry and mid-pass cancellation preserve unwritten outputs", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }

  SECTION("entry cancellation")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroup->getId());
    auto* length = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, output->getId());
    REQUIRE(values != nullptr);
    REQUIRE(length != nullptr);
    length->fill(91);
    const std::atomic_bool shouldCancel = true;
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    const auto executeResult = ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE((*length)[0] == 91);
  }

  SECTION("mid-pass cancellation")
  {
    constexpr usize k_TupleCount = 65537;
    // Small pages split the input into many pages, so the first page read cancels a pass that has more pages to read.
    const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
    std::atomic_bool shouldCancel = false;
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto inputStore = std::make_shared<StatisticsCancelAfterReadDataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{1}, shouldCancel);
    auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
    auto* length = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, output->getId());
    REQUIRE(values != nullptr);
    REQUIRE(length != nullptr);
    length->fill(92);
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(shouldCancel.load());
    REQUIRE((*length)[0] == 92);
  }

  SECTION("between output writes")
  {
    std::atomic_bool shouldCancel = false;
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroup->getId());
    auto lengthStore = std::make_shared<StatisticsCancelAfterWriteDataStore<uint64>>(ShapeType{1}, ShapeType{1}, std::optional<uint64>{0}, shouldCancel);
    auto* length = UInt64Array::Create(dataStructure, "Length", lengthStore, output->getId());
    auto* minimum = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Minimum", {1}, {1}, output->getId());
    REQUIRE(values != nullptr);
    REQUIRE(length != nullptr);
    REQUIRE(minimum != nullptr);
    const std::array<int32, 4> input = {4, 3, 2, 1};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    minimum->fill(93);
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    inputValues.FindMin = true;
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(shouldCancel.load());
    REQUIRE((*length)[0] == 4);
    REQUIRE((*minimum)[0] == 93);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Unique-only without Length", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "Input");
  REQUIRE(group != nullptr);
  auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {5}, {1}, group->getId());
  REQUIRE(values != nullptr);
  const std::array<int32, 5> input = {4, 4, 2, 7, 2};
  auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  scope.requireExpectedStore(*values);
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"})));
  const auto& unique = dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}));
  REQUIRE(unique.getDataStoreRef()[0] == 3);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: bounded exact fallback without external sort", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  if(DataStoreUtilities::GetIOCollection().hasExternalSortCapability())
  {
    SUCCEED("An external-sort provider is registered; the no-provider fallback does not apply.");
    return;
  }
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "Input");
  REQUIRE(group != nullptr);
  auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {6}, {1}, group->getId());
  auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {6}, {1}, group->getId());
  REQUIRE(values != nullptr);
  REQUIRE(mask != nullptr);
  const std::array<int32, 6> input = {5, 1, 5, 3, 3, 9};
  const std::array<bool, 6> maskValues = {true, true, true, true, true, true};
  auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, maskValues);
  SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);

  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Mask"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 6);
  REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Minimum"}))[0] == 1);
  REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Maximum"}))[0] == 9);
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Mean"}))[0] == Approx(26.0F / 6.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == 4.0F);
  REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{3, 5});
  REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Summation"}))[0] == 26.0F);
  REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 4);

  constexpr usize k_UniqueCount = 257;
  DataStructure uniqueData;
  auto* uniqueInputGroup = DataGroup::Create(uniqueData, "Input");
  REQUIRE(uniqueInputGroup != nullptr);
  auto* uniqueInput = Int32Array::CreateWithStore<DataStore<int32>>(uniqueData, "Values", {k_UniqueCount}, {1}, uniqueInputGroup->getId());
  REQUIRE(uniqueInput != nullptr);
  std::array<int32, k_UniqueCount> descending = {};
  std::vector<int32> expectedModes(k_UniqueCount);
  for(usize index = 0; index < k_UniqueCount; ++index)
  {
    descending[index] = static_cast<int32>(k_UniqueCount - index - 1);
    expectedModes[index] = static_cast<int32>(index);
  }
  auto uniqueInputWriteResult = uniqueInput->getDataStoreRef().copyFromBuffer(0, descending);
  SIMPLNX_RESULT_REQUIRE_VALID(uniqueInputWriteResult);
  Arguments uniqueArgs = filter.getDefaultArguments();
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto uniquePreflight = filter.preflight(uniqueData, uniqueArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(uniquePreflight.outputActions);
  auto uniqueResult = scope.executeFilter(filter, uniqueData, uniqueArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(uniqueResult.result);
  REQUIRE(uniqueData.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == k_UniqueCount);
  REQUIRE(uniqueData.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == 128.0F);
  REQUIRE(uniqueData.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == k_UniqueCount);
  REQUIRE(uniqueData.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == expectedModes);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: sparse negative range and empty selection semantics", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  SECTION("ShrinkToFit preserves negative and sparse FeatureIds")
  {
    DataStructure dataStructure;
    auto* group = DataGroup::Create(dataStructure, "Input");
    REQUIRE(group != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {5}, {1}, group->getId());
    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {5}, {1}, group->getId());
    auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {5}, {1}, group->getId());
    REQUIRE(values != nullptr);
    REQUIRE(featureIds != nullptr);
    REQUIRE(mask != nullptr);
    const std::array<int32, 5> input = {10, 20, 30, 40, 50};
    const std::array<int32, 5> ids = {-2, 0, 3, 3, -2};
    const std::array<uint8, 5> maskValues = {1, 1, 1, 0, 1};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto featureIdsWriteResult = featureIds->getDataStoreRef().copyFromBuffer(0, ids);
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
    auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, maskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);

    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), DataPath({"Input", "FeatureIds"}), DataPath({"Input", "Mask"}),
                                                  to_underlying(ComputeArrayStatistics::FeatureIdRangeControls::ShrinkToFit));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::array<int32, 6> expectedMapping = {-2, -1, 0, 1, 2, 3};
    const std::array<uint64, 6> expectedLength = {2, 0, 1, 0, 0, 1};
    const std::array<bool, 6> expectedHasData = {true, false, true, false, false, true};
    std::array<int32, 6> actualMapping = {};
    std::array<uint64, 6> actualLength = {};
    std::array<bool, 6> actualHasData = {};
    auto reducedFeatureIdsIndicesReadResult = dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Reduced Feature Ids Indices"})).getDataStoreRef().copyIntoBuffer(0, actualMapping);
    SIMPLNX_RESULT_REQUIRE_VALID(reducedFeatureIdsIndicesReadResult);
    auto lengthReadResult = dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"})).getDataStoreRef().copyIntoBuffer(0, actualLength);
    SIMPLNX_RESULT_REQUIRE_VALID(lengthReadResult);
    auto featureHasDataReadResult = dataStructure.getDataRefAs<BoolArray>(DataPath({"Statistics", "FeatureHasData"})).getDataStoreRef().copyIntoBuffer(0, actualHasData);
    SIMPLNX_RESULT_REQUIRE_VALID(featureHasDataReadResult);
    REQUIRE(actualMapping == expectedMapping);
    REQUIRE(actualLength == expectedLength);
    REQUIRE(actualHasData == expectedHasData);
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == 30.0F);
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 2);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{10, 50});
    const auto& standardized = dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"}));
    REQUIRE(standardized[0] == Approx(-1.0F));
    REQUIRE(std::isnan(standardized[1]));
    REQUIRE(std::isnan(standardized[2]));
    REQUIRE(standardized[3] == 0.0F);
    REQUIRE(standardized[4] == Approx(1.0F));
  }

  SECTION("all-negative None range is a valid zero-group result")
  {
    DataStructure dataStructure;
    auto* group = DataGroup::Create(dataStructure, "Input");
    REQUIRE(group != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {3}, {1}, group->getId());
    auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {3}, {1}, group->getId());
    REQUIRE(values != nullptr);
    REQUIRE(featureIds != nullptr);
    const std::array<int32, 3> input = {10, 20, 30};
    const std::array<int32, 3> ids = {-3, -2, -1};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto featureIdsWriteResult = featureIds->getDataStoreRef().copyFromBuffer(0, ids);
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
    ComputeArrayStatisticsFilter filter;
    Arguments args = filter.getDefaultArguments();
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Statistics"})).getNumberOfTuples() == 0);
    REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"})).getNumberOfTuples() == 0);
  }

  SECTION("all-false UInt8 mask produces initialized global outputs")
  {
    DataStructure dataStructure;
    auto* group = DataGroup::Create(dataStructure, "Input");
    REQUIRE(group != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, group->getId());
    auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {4}, {1}, group->getId());
    REQUIRE(values != nullptr);
    REQUIRE(mask != nullptr);
    const std::array<int32, 4> input = {1, 2, 3, 4};
    const std::array<uint8, 4> maskValues = {0, 0, 0, 0};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto maskWriteResult = mask->getDataStoreRef().copyFromBuffer(0, maskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), {}, DataPath({"Input", "Mask"}));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Minimum"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Maximum"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Mean"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Median"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "StandardDeviation"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Statistics", "Summation"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0).empty());
    std::array<float32, 4> standardized = {};
    auto standardizedReadResult = dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, standardized);
    SIMPLNX_RESULT_REQUIRE_VALID(standardizedReadResult);
    REQUIRE(standardized == std::array<float32, 4>{0, 0, 0, 0});
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Scanline range and degenerate edges", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }

  SECTION("zero tuples")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    REQUIRE(inputGroup != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {0}, {1}, inputGroup->getId());
    REQUIRE(values != nullptr);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0).empty());
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getNumberOfTuples() == 0);
  }

  SECTION("Custom and padded maximum minus one use the observed maximum")
  {
    const auto runRange = [&scanlineScenario](ComputeArrayStatistics::FeatureIdRangeControls rangeType, std::vector<int32> range) {
      DataStructure dataStructure;
      auto* inputGroup = DataGroup::Create(dataStructure, "Input");
      REQUIRE(inputGroup != nullptr);
      auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroup->getId());
      auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {4}, {1}, inputGroup->getId());
      REQUIRE(values != nullptr);
      REQUIRE(featureIds != nullptr);
      const std::array<int32, 4> input = {2, 5, 5, 9};
      const std::array<int32, 4> ids = {2, 5, 5, 9};
      auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
      SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
      auto featureIdsWriteResult = featureIds->getDataStoreRef().copyFromBuffer(0, ids);
      SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
      ComputeArrayStatisticsFilter filter;
      Arguments args = filter.getDefaultArguments();
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(rangeType)));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<std::vector<int32>>(std::move(range)));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      UnitTest::AlgorithmTestScope scope(*scanlineScenario);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      const auto& mapping = dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Reduced Feature Ids Indices"}));
      const auto& length = dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}));
      std::vector<int32> mappingValues(mapping.getNumberOfTuples());
      std::vector<uint64> lengthValues(length.getNumberOfTuples());
      auto mappingReadResult = mapping.getDataStoreRef().copyIntoBuffer(0, mappingValues);
      SIMPLNX_RESULT_REQUIRE_VALID(mappingReadResult);
      auto lengthReadResult = length.getDataStoreRef().copyIntoBuffer(0, lengthValues);
      SIMPLNX_RESULT_REQUIRE_VALID(lengthReadResult);
      return std::make_pair(std::move(mappingValues), std::move(lengthValues));
    };

    const auto custom = runRange(ComputeArrayStatistics::FeatureIdRangeControls::CustomRange, {3, -1});
    REQUIRE(custom.first == std::vector<int32>{3, 4, 5, 6, 7, 8, 9});
    REQUIRE(custom.second == std::vector<uint64>{0, 0, 2, 0, 0, 0, 1});
    const auto padded = runRange(ComputeArrayStatistics::FeatureIdRangeControls::PaddedCustomRange, {1, -1});
    REQUIRE(padded.first == std::vector<int32>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    REQUIRE(padded.second == std::vector<uint64>{0, 1, 0, 0, 2, 0, 0, 0, 1});
  }

  SECTION("range errors and FeatureId span overflow")
  {
    const auto runInvalidRange = [](const std::array<int32, 2>& ids, ComputeArrayStatistics::FeatureIdRangeControls rangeType, std::vector<int32> range) {
      DataStructure dataStructure;
      auto* inputGroup = DataGroup::Create(dataStructure, "Input");
      REQUIRE(inputGroup != nullptr);
      auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {2}, {1}, inputGroup->getId());
      auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {2}, {1}, inputGroup->getId());
      REQUIRE(values != nullptr);
      REQUIRE(featureIds != nullptr);
      const std::array<int32, 2> input = {1, 2};
      auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
      SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
      auto featureIdsWriteResult = featureIds->getDataStoreRef().copyFromBuffer(0, ids);
      SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
      ComputeArrayStatisticsFilter filter;
      Arguments args = filter.getDefaultArguments();
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(rangeType)));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<std::vector<int32>>(std::move(range)));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
      args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      auto executeResult = filter.execute(dataStructure, args);
      REQUIRE(executeResult.result.invalid());
      return executeResult.result.errors().front().code;
    };

    REQUIRE(runInvalidRange({2, 9}, ComputeArrayStatistics::FeatureIdRangeControls::CustomRange, {10, 12}) == -506671);
    REQUIRE(runInvalidRange({std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max()}, ComputeArrayStatistics::FeatureIdRangeControls::ShrinkToFit, {0, -1}) == -57300);
  }

  SECTION("constant values and numeric extremes")
  {
    DataStructure constantData;
    auto* inputGroup = DataGroup::Create(constantData, "Input");
    REQUIRE(inputGroup != nullptr);
    auto* values = Int32Array::CreateWithStore<DataStore<int32>>(constantData, "Values", {3}, {1}, inputGroup->getId());
    REQUIRE(values != nullptr);
    const std::array<int32, 3> input = {7, 7, 7};
    auto valuesWriteResult = values->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    auto preflightResult = filter.preflight(constantData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    UnitTest::AlgorithmTestScope constantScope(*scanlineScenario);
    auto executeResult = constantScope.executeFilter(filter, constantData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(constantData.getDataRefAs<Float32Array>(DataPath({"Statistics", "StandardDeviation"}))[0] == 0.0F);
    REQUIRE(constantData.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{7});
    const auto& standardized = constantData.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"}));
    REQUIRE(std::isnan(standardized[0]));
    REQUIRE(std::isnan(standardized[1]));
    REQUIRE(std::isnan(standardized[2]));

    DataStructure extremeData;
    auto* extremeInputGroup = DataGroup::Create(extremeData, "Input");
    REQUIRE(extremeInputGroup != nullptr);
    auto* extremes = Int64Array::CreateWithStore<DataStore<int64>>(extremeData, "Values", {2}, {1}, extremeInputGroup->getId());
    REQUIRE(extremes != nullptr);
    const std::array<int64, 2> extremeInput = {std::numeric_limits<int64>::lowest(), std::numeric_limits<int64>::max()};
    auto extremesWriteResult = extremes->getDataStoreRef().copyFromBuffer(0, extremeInput);
    SIMPLNX_RESULT_REQUIRE_VALID(extremesWriteResult);
    Arguments extremeArgs = filter.getDefaultArguments();
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
    auto extremePreflight = filter.preflight(extremeData, extremeArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(extremePreflight.outputActions);
    UnitTest::AlgorithmTestScope extremeScope(*scanlineScenario);
    auto extremeResult = extremeScope.executeFilter(filter, extremeData, extremeArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(extremeResult.result);
    REQUIRE(extremeData.getDataRefAs<Int64Array>(DataPath({"Statistics", "Minimum"}))[0] == std::numeric_limits<int64>::lowest());
    REQUIRE(extremeData.getDataRefAs<Int64Array>(DataPath({"Statistics", "Maximum"}))[0] == std::numeric_limits<int64>::max());
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: every supported scalar type", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  RunCompactNumericStatistics<int8>(scope);
  RunCompactNumericStatistics<uint8>(scope);
  RunCompactNumericStatistics<int16>(scope);
  RunCompactNumericStatistics<uint16>(scope);
  RunCompactNumericStatistics<int32>(scope);
  RunCompactNumericStatistics<uint32>(scope);
  RunCompactNumericStatistics<int64>(scope);
  RunCompactNumericStatistics<uint64>(scope);
  RunCompactNumericStatistics<float32>(scope);
  RunCompactNumericStatistics<float64>(scope);
  RunCompactNumericStatistics<bool>(scope);
}

namespace
{

/**
 * @enum WholeArrayMask
 * @brief Selects the mask array of one whole-array test case.
 */
enum class WholeArrayMask
{
  None,
  Bool,
  UInt8,
  AllMasked
};

/**
 * @brief Returns whether tuple index is selected by a test mask pattern.
 */
bool IsSelectedByTestMask(WholeArrayMask maskKind, usize index)
{
  switch(maskKind)
  {
  case WholeArrayMask::None:
    return true;
  case WholeArrayMask::Bool:
    return index % 3 != 1;
  case WholeArrayMask::UInt8:
    return index % 5 < 3;
  case WholeArrayMask::AllMasked:
    return false;
  }
  return true;
}

/**
 * @brief Returns the number of pages that a whole-array reduction pass reads with the test seams.
 * @param tupleCount Input tuples.
 * @param bytesPerTuple Value bytes plus mask bytes for one tuple.
 */
usize ExpectedReductionPages(usize tupleCount, usize bytesPerTuple)
{
  const usize pageTuples = std::max(k_SeamBlockTuples, (k_SeamPageBytes / bytesPerTuple / k_SeamBlockTuples) * k_SeamBlockTuples);
  return (tupleCount + pageTuples - 1) / pageTuples;
}

/**
 * @brief Deterministic 64-bit mix of a tuple index for synthetic input.
 */
uint64 MixIndex(usize index)
{
  uint64 value = static_cast<uint64>(index) + 0x9E3779B97F4A7C15ULL;
  value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31);
}

/**
 * @brief Creates synthetic input with ties, negative values for signed types, and large magnitudes for wide types.
 */
template <typename T>
std::vector<T> MakeWholeArrayInput(usize tupleCount)
{
  std::vector<T> values(tupleCount);
  for(usize index = 0; index < tupleCount; ++index)
  {
    const uint64 mixed = MixIndex(index);
    if constexpr(std::is_same_v<T, bool>)
    {
      values[index] = (mixed % 7) < 3;
    }
    else if constexpr(std::is_integral_v<T> && sizeof(T) <= 2)
    {
      // 97 levels from the lowest to the highest value give ties, negative values for signed types, and both extremes.
      const int64 level = static_cast<int64>(mixed % 97);
      const int64 low = static_cast<int64>(std::numeric_limits<T>::min());
      const int64 high = static_cast<int64>(std::numeric_limits<T>::max());
      values[index] = static_cast<T>(low + level * (high - low) / 96);
    }
    else if constexpr(std::is_same_v<T, int32>)
    {
      values[index] = static_cast<T>(static_cast<int64>(mixed % 2000000000ULL) - 1000000000LL);
    }
    else if constexpr(std::is_same_v<T, uint32>)
    {
      // Values near 4e9 are where the Float32 mean quantization is visible.
      values[index] = static_cast<T>(4000000000ULL + mixed % 290000000ULL);
    }
    else if constexpr(std::is_same_v<T, int64>)
    {
      values[index] = static_cast<T>(static_cast<int64>(mixed % 2000000000000000ULL) - 1000000000000000LL);
    }
    else if constexpr(std::is_same_v<T, uint64>)
    {
      values[index] = static_cast<T>(mixed % 1000000000000000ULL);
    }
    else
    {
      values[index] = static_cast<T>(static_cast<float64>(static_cast<int64>(mixed % 2000001ULL) - 900000) / 7.0);
    }
  }
  return values;
}

/**
 * @struct WholeArrayResult
 * @brief Whole-array statistics read back from one filter execution.
 */
template <typename T>
struct WholeArrayResult
{
  uint64 Length = 0;
  T Minimum = {};
  T Maximum = {};
  float32 Mean = 0.0F;
  float32 Median = 0.0F;
  float32 StandardDeviation = 0.0F;
  float32 Summation = 0.0F;
  int32 Unique = 0;
  std::vector<T> Modes;
  std::vector<float32> Standardized;
};

/**
 * @brief Writes test values to a store through one bulk write.
 */
template <typename T>
void WriteTestValues(AbstractDataStore<T>& store, const std::vector<T>& values)
{
  auto buffer = std::make_unique<T[]>(values.size());
  std::copy(values.cbegin(), values.cend(), buffer.get());
  auto writeResult = store.copyFromBuffer(0, nonstd::span<const T>(buffer.get(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
}

/**
 * @enum BoolBytes
 * @brief Selects the byte that a test Boolean store holds for each true value.
 */
enum class BoolBytes
{
  Canonical,   ///< Byte 1 for each true value.
  NonCanonical ///< A nonzero byte from {1, 2, 127, 128, 255} for each true value, as h5py, MATLAB, or legacy writers can store.
};

/**
 * @brief Writes Boolean test values to a store through one bulk write.
 * @param store Destination store.
 * @param values Canonical values that the oracles use.
 * @param boolBytes Byte that the store holds for each true value.
 *
 * Non-canonical bytes go into the bool buffer through memcpy, so no bool conversion changes them. The test then
 * reads the store back and requires the same bytes. Thus the engines under test receive non-canonical bytes.
 */
void WriteBoolTestValues(AbstractDataStore<bool>& store, const std::vector<bool>& values, BoolBytes boolBytes)
{
  if(boolBytes == BoolBytes::Canonical)
  {
    WriteTestValues(store, values);
    return;
  }
  static_assert(sizeof(bool) == 1, "The non-canonical byte pattern requires one-byte bool objects.");
  constexpr std::array<uint8, 5> k_TrueBytes = {1, 2, 127, 128, 255};
  std::vector<uint8> bytes(values.size());
  for(usize index = 0; index < values.size(); ++index)
  {
    bytes[index] = values[index] ? k_TrueBytes[index % k_TrueBytes.size()] : 0;
  }
  auto buffer = std::make_unique<bool[]>(bytes.size());
  std::memcpy(buffer.get(), bytes.data(), bytes.size());
  auto writeResult = store.copyFromBuffer(0, nonstd::span<const bool>(buffer.get(), bytes.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  auto readBack = std::make_unique<bool[]>(bytes.size());
  auto readResult = store.copyIntoBuffer(0, nonstd::span<bool>(readBack.get(), bytes.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  REQUIRE(std::memcmp(readBack.get(), bytes.data(), bytes.size()) == 0);
}

/**
 * @brief Creates one array with an in-memory store, or with the out-of-core store that the ForceOutOfCore preference selects.
 * @pre With preferenceStore, the caller set the ForceOutOfCore storage preference.
 */
template <typename T>
DataArray<T>* CreateWholeArrayInput(DataStructure& dataStructure, const std::string& name, usize tupleCount, bool preferenceStore, DataObject::IdType parentId)
{
  if(!preferenceStore)
  {
    return DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, name, {tupleCount}, {1}, parentId);
  }
  auto store = DataStoreUtilities::CreateDataStore<T>(dataStructure, DataPath({"Input", name}), {tupleCount}, {1});
  REQUIRE(store != nullptr);
  REQUIRE(store->getStoreType() == IDataStore::StoreType::OutOfCore);
  return DataArray<T>::Create(dataStructure, name, store, parentId);
}

/**
 * @brief Runs the filter on one ungrouped input and reads back every whole-array output.
 * @param execute Runs the filter; it receives the filter, the DataStructure, and the arguments.
 * @param values Input values.
 * @param maskKind Mask pattern, if any.
 * @param orderStatistics True to request median, unique count, and (for integer input) mode.
 * @param preferenceStores True to create input stores through the storage preferences instead of as in-memory stores.
 * @param boolBytes Byte that Boolean input and Boolean mask stores hold for each true value.
 */
template <typename T, typename ExecuteT>
WholeArrayResult<T> RunWholeArrayCase(const ExecuteT& execute, const std::vector<T>& values, WholeArrayMask maskKind, bool orderStatistics, bool preferenceStores = false,
                                      BoolBytes boolBytes = BoolBytes::Canonical)
{
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroup != nullptr);
  auto* inputArray = CreateWholeArrayInput<T>(dataStructure, "Values", values.size(), preferenceStores, inputGroup->getId());
  REQUIRE(inputArray != nullptr);
  if constexpr(std::is_same_v<T, bool>)
  {
    WriteBoolTestValues(inputArray->getDataStoreRef(), values, boolBytes);
  }
  else
  {
    WriteTestValues(inputArray->getDataStoreRef(), values);
  }
  std::optional<DataPath> maskPath;
  if(maskKind == WholeArrayMask::Bool)
  {
    auto* mask = CreateWholeArrayInput<bool>(dataStructure, "Mask", values.size(), preferenceStores, inputGroup->getId());
    REQUIRE(mask != nullptr);
    std::vector<bool> maskValues(values.size());
    for(usize index = 0; index < values.size(); ++index)
    {
      maskValues[index] = IsSelectedByTestMask(maskKind, index);
    }
    WriteBoolTestValues(mask->getDataStoreRef(), maskValues, boolBytes);
    maskPath = DataPath({"Input", "Mask"});
  }
  else if(maskKind == WholeArrayMask::UInt8 || maskKind == WholeArrayMask::AllMasked)
  {
    auto* mask = CreateWholeArrayInput<uint8>(dataStructure, "Mask", values.size(), preferenceStores, inputGroup->getId());
    REQUIRE(mask != nullptr);
    std::vector<uint8> maskValues(values.size());
    for(usize index = 0; index < values.size(); ++index)
    {
      // Nonzero values other than 1 must also select a tuple.
      maskValues[index] = IsSelectedByTestMask(maskKind, index) ? static_cast<uint8>(1 + index % 3) : 0;
    }
    WriteTestValues(mask->getDataStoreRef(), maskValues);
    maskPath = DataPath({"Input", "Mask"});
  }

  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(orderStatistics));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(orderStatistics));
  constexpr bool k_SupportsMode = std::is_integral_v<T> && !std::is_same_v<T, bool>;
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(orderStatistics && k_SupportsMode));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  if(maskPath.has_value())
  {
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(*maskPath));
  }
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = execute(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath statisticsPath({"Statistics"});
  WholeArrayResult<T> result;
  result.Length = dataStructure.getDataRefAs<UInt64Array>(statisticsPath.createChildPath("Length"))[0];
  result.Minimum = dataStructure.getDataRefAs<DataArray<T>>(statisticsPath.createChildPath("Minimum"))[0];
  result.Maximum = dataStructure.getDataRefAs<DataArray<T>>(statisticsPath.createChildPath("Maximum"))[0];
  result.Mean = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Mean"))[0];
  result.StandardDeviation = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("StandardDeviation"))[0];
  result.Summation = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Summation"))[0];
  if(orderStatistics)
  {
    result.Median = dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Median"))[0];
    result.Unique = dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("NumUniqueValues"))[0];
    if constexpr(k_SupportsMode)
    {
      result.Modes = dataStructure.getDataRefAs<NeighborList<T>>(statisticsPath.createChildPath("Mode")).getList(0);
    }
  }
  result.Standardized.resize(values.size());
  auto standardizedReadResult = dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, result.Standardized);
  SIMPLNX_RESULT_REQUIRE_VALID(standardizedReadResult);
  return result;
}

/**
 * @struct TodayScanlineOracle
 * @brief Whole-array statistics computed with the element-order formulas of the bounded implementation.
 *
 * Min and max use strict comparisons against the max() and lowest() sentinels, so NaN is ignored and the first
 * of equal values is kept. The sum uses Int64, UInt64, or Float64 in element order. The mean divides the Float32
 * sum by the Float32 length. The deviation accumulates Float64(value - mean) squared in element order, where the
 * subtraction uses the promoted type of value and the Float32 mean.
 */
template <typename T>
struct TodayScanlineOracle
{
  uint64 Length = 0;
  T Minimum = {};
  T Maximum = {};
  float32 Summation = 0.0F;
  float32 Mean = 0.0F;
  float32 StandardDeviation = 0.0F;
  float32 Median = 0.0F;
  int32 Unique = 0;
  std::vector<T> Modes;
  float64 AbsoluteSum = 0.0; ///< Sum of |x| over the selected values, for the summation regrouping bound.
};

template <typename T>
TodayScanlineOracle<T> ComputeTodayScanlineOracle(const std::vector<T>& values, WholeArrayMask maskKind)
{
  using SumType = std::conditional_t<std::is_integral_v<T>, std::conditional_t<std::is_signed_v<T>, int64, uint64>, float64>;
  std::vector<T> selected;
  for(usize index = 0; index < values.size(); ++index)
  {
    if(IsSelectedByTestMask(maskKind, index))
    {
      selected.push_back(values[index]);
    }
  }
  TodayScanlineOracle<T> oracle;
  oracle.Length = selected.size();
  if(selected.empty())
  {
    return oracle;
  }
  T minimum = std::numeric_limits<T>::max();
  T maximum = std::numeric_limits<T>::lowest();
  SumType sum = 0;
  for(const T value : selected)
  {
    if(value < minimum)
    {
      minimum = value;
    }
    if(value > maximum)
    {
      maximum = value;
    }
    sum += static_cast<SumType>(value);
    oracle.AbsoluteSum += std::fabs(static_cast<float64>(value));
  }
  oracle.Minimum = minimum;
  oracle.Maximum = maximum;
  oracle.Summation = static_cast<float32>(sum);
  oracle.Mean = oracle.Summation / static_cast<float32>(selected.size());
  float64 sumOfDifferences = 0.0;
  for(const T value : selected)
  {
    const float64 difference = static_cast<float64>(value - oracle.Mean);
    sumOfDifferences += difference * difference;
  }
  oracle.StandardDeviation = static_cast<float32>(std::sqrt(sumOfDifferences / static_cast<float64>(selected.size())));

  if constexpr(std::is_integral_v<T>)
  {
    std::sort(selected.begin(), selected.end());
    const T lower = selected[(selected.size() - 1) / 2];
    const T upper = selected[selected.size() / 2];
    oracle.Median = selected.size() % 2 == 1 ? static_cast<float32>(upper) : static_cast<float32>((lower + upper) * 0.5F);
    usize maximumRun = 0;
    for(usize index = 0; index < selected.size();)
    {
      usize next = index;
      while(next < selected.size() && selected[next] == selected[index])
      {
        ++next;
      }
      const usize run = next - index;
      if(run > maximumRun)
      {
        maximumRun = run;
        oracle.Modes.clear();
      }
      if(run == maximumRun)
      {
        oracle.Modes.push_back(selected[index]);
      }
      ++oracle.Unique;
      index = next;
    }
  }
  return oracle;
}

/**
 * @brief Compares two Float32 results that may differ by regrouped Float64 summation and one Float32 rounding.
 * @param actual Result under test.
 * @param expected Oracle result.
 * @param absoluteBound Largest Float64 difference that regrouped summation can produce before the Float32 cast.
 * @return True for equal NaN state and a difference within the bound plus two Float32 units in the last place.
 */
bool WithinRegroupedTolerance(float32 actual, float32 expected, float64 absoluteBound)
{
  if(std::isnan(expected) || std::isnan(actual))
  {
    return std::isnan(expected) && std::isnan(actual);
  }
  if(std::isinf(expected) || std::isinf(actual))
  {
    return actual == expected;
  }
  const float64 unitInLastPlace = std::fabs(static_cast<float64>(expected)) * static_cast<float64>(std::numeric_limits<float32>::epsilon());
  return std::fabs(static_cast<float64>(actual) - static_cast<float64>(expected)) <= absoluteBound + 2.0 * unitInLastPlace;
}

/**
 * @brief Checks the standardized output against the formula applied with the reported mean and deviation.
 *
 * Unselected tuples keep the value 0 that the created output array starts with.
 */
template <typename T>
void RequireStandardizedMatchesFormula(const std::vector<T>& values, WholeArrayMask maskKind, const WholeArrayResult<T>& actual)
{
  REQUIRE(actual.Standardized.size() == values.size());
  for(usize index = 0; index < values.size(); ++index)
  {
    const float32 expected = IsSelectedByTestMask(maskKind, index) ? (static_cast<float32>(values[index]) - actual.Mean) / actual.StandardDeviation : 0.0F;
    if(std::isnan(expected))
    {
      CHECK(std::isnan(actual.Standardized[index]));
    }
    else
    {
      CHECK(actual.Standardized[index] == expected);
    }
  }
}

/**
 * @brief Compares results of the histogram engine with the oracle: exact except the deviation.
 */
template <typename T>
void RequireHistogramResultMatchesOracle(const std::vector<T>& values, WholeArrayMask maskKind, const WholeArrayResult<T>& actual)
{
  const TodayScanlineOracle<T> expected = ComputeTodayScanlineOracle(values, maskKind);
  REQUIRE(actual.Length == expected.Length);
  REQUIRE(actual.Minimum == expected.Minimum);
  REQUIRE(actual.Maximum == expected.Maximum);
  REQUIRE(actual.Summation == expected.Summation);
  REQUIRE(actual.Mean == expected.Mean);
  REQUIRE(actual.Median == expected.Median);
  REQUIRE(actual.Unique == expected.Unique);
  if constexpr(!std::is_same_v<T, bool>)
  {
    REQUIRE(actual.Modes == expected.Modes);
  }
  // Only the summation order differs from the element-order oracle.
  REQUIRE(actual.StandardDeviation == Approx(expected.StandardDeviation).epsilon(1.0e-6));
  RequireStandardizedMatchesFormula(values, maskKind, actual);
}

/**
 * @brief Compares results of the two-pass engine with the oracle.
 *
 * Integer extrema and sums are exact. Float sums may differ by regrouped Float64 summation, which is bounded by
 * n * eps * sum(|x|) before the Float32 cast.
 */
template <typename T>
void RequireTwoPassResultMatchesOracle(const std::vector<T>& values, WholeArrayMask maskKind, const WholeArrayResult<T>& actual)
{
  const TodayScanlineOracle<T> expected = ComputeTodayScanlineOracle(values, maskKind);
  REQUIRE(actual.Length == expected.Length);
  REQUIRE(actual.Minimum == expected.Minimum);
  REQUIRE(actual.Maximum == expected.Maximum);
  if constexpr(std::is_integral_v<T>)
  {
    REQUIRE(actual.Summation == expected.Summation);
    REQUIRE(actual.Mean == expected.Mean);
  }
  else
  {
    const float64 sumBound = static_cast<float64>(expected.Length) * std::numeric_limits<float64>::epsilon() * expected.AbsoluteSum;
    CHECK(WithinRegroupedTolerance(actual.Summation, expected.Summation, sumBound));
    const float64 meanBound = expected.Length == 0 ? 0.0 : sumBound / static_cast<float64>(expected.Length) + std::fabs(static_cast<float64>(expected.Mean)) * 1.0e-6;
    CHECK(WithinRegroupedTolerance(actual.Mean, expected.Mean, meanBound));
  }
  REQUIRE(actual.StandardDeviation == Approx(expected.StandardDeviation).epsilon(1.0e-6));
  RequireStandardizedMatchesFormula(values, maskKind, actual);
}

/**
 * @brief Runs every length and mask case of the histogram engine for one type on the scanline path.
 */
template <typename T>
void RunHistogramOracleCases(UnitTest::AlgorithmTestScenario scanlineScenario)
{
  // One tuple, one partial page, several pages with a partial last page, and a larger multi-page case.
  for(const usize tupleCount : {usize{1}, usize{7}, usize{1000}, usize{4099}})
  {
    for(const WholeArrayMask maskKind : {WholeArrayMask::None, WholeArrayMask::Bool, WholeArrayMask::UInt8, WholeArrayMask::AllMasked})
    {
      CAPTURE(std::string(DataTypeToString(GetDataType<T>()).view()), tupleCount, static_cast<int32>(maskKind));
      const std::vector<T> values = MakeWholeArrayInput<T>(tupleCount);
      const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
      UnitTest::AlgorithmTestScope scope(scanlineScenario);
      const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
      const WholeArrayResult<T> actual = RunWholeArrayCase<T>(execute, values, maskKind, true);
      RequireHistogramResultMatchesOracle(values, maskKind, actual);

      const usize maskBytes = maskKind == WholeArrayMask::None ? 0 : (maskKind == WholeArrayMask::Bool ? sizeof(bool) : sizeof(uint8));
      const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
      REQUIRE(witness.HistogramPages == ExpectedReductionPages(tupleCount, sizeof(T) + maskBytes));
      REQUIRE(witness.MomentPages == 0);
      REQUIRE(witness.DeviationPages == 0);
      REQUIRE(witness.StandardizePages > 0);
    }
  }
}

/**
 * @brief Runs every length and mask case of the two-pass engine for one type on the scanline path.
 */
template <typename T>
void RunTwoPassOracleCases(UnitTest::AlgorithmTestScenario scanlineScenario)
{
  for(const usize tupleCount : {usize{1}, usize{7}, usize{1000}, usize{4099}})
  {
    for(const WholeArrayMask maskKind : {WholeArrayMask::None, WholeArrayMask::Bool, WholeArrayMask::UInt8, WholeArrayMask::AllMasked})
    {
      CAPTURE(std::string(DataTypeToString(GetDataType<T>()).view()), tupleCount, static_cast<int32>(maskKind));
      const std::vector<T> values = MakeWholeArrayInput<T>(tupleCount);
      const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
      UnitTest::AlgorithmTestScope scope(scanlineScenario);
      const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
      const WholeArrayResult<T> actual = RunWholeArrayCase<T>(execute, values, maskKind, false);
      RequireTwoPassResultMatchesOracle(values, maskKind, actual);

      const usize maskBytes = maskKind == WholeArrayMask::None ? 0 : (maskKind == WholeArrayMask::Bool ? sizeof(bool) : sizeof(uint8));
      const usize expectedPages = ExpectedReductionPages(tupleCount, sizeof(T) + maskBytes);
      const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
      REQUIRE(witness.HistogramPages == 0);
      REQUIRE(witness.MomentPages == expectedPages);
      // The deviation pass runs only when a tuple is selected.
      REQUIRE(witness.DeviationPages == (maskKind == WholeArrayMask::AllMasked ? 0 : expectedPages));
      REQUIRE(witness.StandardizePages > 0);
    }
  }
}

/**
 * @brief Returns true when two Float32 values have identical bits.
 */
bool SameBits(float32 left, float32 right)
{
  return std::memcmp(&left, &right, sizeof(float32)) == 0;
}

/**
 * @brief Requires bit-identical whole-array results.
 */
template <typename T>
void RequireBitIdentical(const WholeArrayResult<T>& left, const WholeArrayResult<T>& right)
{
  REQUIRE(left.Length == right.Length);
  REQUIRE(std::memcmp(&left.Minimum, &right.Minimum, sizeof(T)) == 0);
  REQUIRE(std::memcmp(&left.Maximum, &right.Maximum, sizeof(T)) == 0);
  REQUIRE(SameBits(left.Mean, right.Mean));
  REQUIRE(SameBits(left.Median, right.Median));
  REQUIRE(SameBits(left.StandardDeviation, right.StandardDeviation));
  REQUIRE(SameBits(left.Summation, right.Summation));
  REQUIRE(left.Unique == right.Unique);
  REQUIRE(left.Modes == right.Modes);
  REQUIRE(left.Standardized.size() == right.Standardized.size());
  REQUIRE(std::memcmp(left.Standardized.data(), right.Standardized.data(), left.Standardized.size() * sizeof(float32)) == 0);
}

/**
 * @brief Runs one scanline whole-array case with the given page budget and optional TBB thread limit.
 * @param threadLimit Maximum TBB parallelism, or 0 for the default.
 */
template <typename T>
WholeArrayResult<T> RunScanlineWithLimits(UnitTest::AlgorithmTestScenario scanlineScenario, const std::vector<T>& values, bool orderStatistics, usize pageBytes, usize threadLimit)
{
#ifdef SIMPLNX_ENABLE_MULTICORE
  std::optional<tbb::global_control> parallelismLimit;
  if(threadLimit != 0)
  {
    parallelismLimit.emplace(tbb::global_control::max_allowed_parallelism, threadLimit);
  }
#endif
  const ScanSeamScope seams(pageBytes, k_SeamBlockTuples);
  UnitTest::AlgorithmTestScope scope(scanlineScenario);
  const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
  return RunWholeArrayCase<T>(execute, values, WholeArrayMask::Bool, orderStatistics);
}

/**
 * @brief Requires that every nonzero Boolean byte counts as one true value in each whole-array engine.
 * @param execute Runs the filter; it receives the filter, the DataStructure, and the arguments.
 * @param tupleCount Input tuples.
 * @param preferenceStores True to create the input and mask stores through the storage preferences.
 *
 * The oracles use the canonical values. The stores hold the bytes {1, 2, 127, 128, 255} for true values. A
 * non-canonical byte that reaches an engine as a bool object gives a counter index past the Boolean histogram,
 * or adds a selection weight larger than one.
 */
template <typename ExecuteT>
void RequireNonCanonicalBoolBytesCountOnce(const ExecuteT& execute, usize tupleCount, bool preferenceStores)
{
  {
    INFO("Boolean input on the histogram engine");
    const std::vector<bool> values = MakeWholeArrayInput<bool>(tupleCount);
    const uint64 trueCount = static_cast<uint64>(std::count(values.cbegin(), values.cend(), true));
    REQUIRE(trueCount > 0);
    REQUIRE(trueCount < tupleCount);
    const auto actual = RunWholeArrayCase<bool>(execute, values, WholeArrayMask::None, true, preferenceStores, BoolBytes::NonCanonical);
    REQUIRE(actual.Length == tupleCount);
    REQUIRE(actual.Summation == static_cast<float32>(trueCount));
    REQUIRE(actual.Mean == static_cast<float32>(trueCount) / static_cast<float32>(tupleCount));
    REQUIRE(actual.Minimum == false);
    REQUIRE(actual.Maximum == true);
    REQUIRE(actual.Unique == 2);
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::None, actual);
  }
  {
    INFO("UInt16 input with a Boolean mask on the histogram engine");
    const std::vector<uint16> values = MakeWholeArrayInput<uint16>(tupleCount);
    const auto actual = RunWholeArrayCase<uint16>(execute, values, WholeArrayMask::Bool, true, preferenceStores, BoolBytes::NonCanonical);
    const TodayScanlineOracle<uint16> expected = ComputeTodayScanlineOracle(values, WholeArrayMask::Bool);
    REQUIRE(actual.Length == expected.Length);
    REQUIRE(actual.Summation == expected.Summation);
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::Bool, actual);
  }
  {
    INFO("Int32 input with a Boolean mask on the two-pass engine");
    const std::vector<int32> values = MakeWholeArrayInput<int32>(tupleCount);
    const auto actual = RunWholeArrayCase<int32>(execute, values, WholeArrayMask::Bool, false, preferenceStores, BoolBytes::NonCanonical);
    RequireTwoPassResultMatchesOracle(values, WholeArrayMask::Bool, actual);
  }
  {
    INFO("Int32 input with a Boolean mask on the sorted exact path");
    const std::vector<int32> values = MakeWholeArrayInput<int32>(tupleCount);
    const auto actual = RunWholeArrayCase<int32>(execute, values, WholeArrayMask::Bool, true, preferenceStores, BoolBytes::NonCanonical);
    const TodayScanlineOracle<int32> expected = ComputeTodayScanlineOracle(values, WholeArrayMask::Bool);
    REQUIRE(actual.Length == expected.Length);
    REQUIRE(actual.Minimum == expected.Minimum);
    REQUIRE(actual.Maximum == expected.Maximum);
    REQUIRE(actual.Summation == expected.Summation);
    REQUIRE(actual.Mean == expected.Mean);
    REQUIRE(actual.Median == expected.Median);
    REQUIRE(actual.Unique == expected.Unique);
    REQUIRE(actual.Modes == expected.Modes);
    REQUIRE(actual.StandardDeviation == expected.StandardDeviation);
    RequireStandardizedMatchesFormula(values, WholeArrayMask::Bool, actual);
  }
}
} // namespace

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array histogram engine matches the scanline oracle", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  RunHistogramOracleCases<bool>(*scanlineScenario);
  RunHistogramOracleCases<int8>(*scanlineScenario);
  RunHistogramOracleCases<uint8>(*scanlineScenario);
  RunHistogramOracleCases<int16>(*scanlineScenario);
  RunHistogramOracleCases<uint16>(*scanlineScenario);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array histogram order statistics", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };

  SECTION("signed values with two tied modes and an odd length")
  {
    const std::vector<int16> values = {-300, 7, -300, 12, 7, -32768, 32767};
    const auto actual = RunWholeArrayCase<int16>(execute, values, WholeArrayMask::None, true);
    REQUIRE(actual.Minimum == -32768);
    REQUIRE(actual.Maximum == 32767);
    REQUIRE(actual.Median == 7.0F);
    REQUIRE(actual.Unique == 5);
    REQUIRE(actual.Modes == std::vector<int16>{-300, 7});
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::None, actual);
  }

  SECTION("negative int8 values with an even length average the middle pair")
  {
    const std::vector<int8> values = {-128, -5, -4, 127, -5, -4};
    const auto actual = RunWholeArrayCase<int8>(execute, values, WholeArrayMask::None, true);
    REQUIRE(actual.Median == -4.5F);
    REQUIRE(actual.Modes == std::vector<int8>{-5, -4});
    REQUIRE(actual.Unique == 4);
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::None, actual);
  }

  SECTION("every value is a mode")
  {
    const std::vector<uint8> values = {9, 3, 255, 0};
    const auto actual = RunWholeArrayCase<uint8>(execute, values, WholeArrayMask::None, true);
    REQUIRE(actual.Median == 6.0F);
    REQUIRE(actual.Modes == std::vector<uint8>{0, 3, 9, 255});
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::None, actual);
  }

  SECTION("Boolean median with even and odd lengths")
  {
    const std::vector<bool> evenValues = {true, false, false, true};
    const auto even = RunWholeArrayCase<bool>(execute, evenValues, WholeArrayMask::None, true);
    REQUIRE(even.Median == 0.5F);
    REQUIRE(even.Unique == 2);
    RequireHistogramResultMatchesOracle(evenValues, WholeArrayMask::None, even);
    const std::vector<bool> oddValues = {true, false, true};
    const auto odd = RunWholeArrayCase<bool>(execute, oddValues, WholeArrayMask::None, true);
    REQUIRE(odd.Median == 1.0F);
    RequireHistogramResultMatchesOracle(oddValues, WholeArrayMask::None, odd);
  }

  SECTION("all-masked input has no mode entries and zero extrema")
  {
    const std::vector<uint16> values = {4, 5, 6};
    const auto actual = RunWholeArrayCase<uint16>(execute, values, WholeArrayMask::AllMasked, true);
    REQUIRE(actual.Length == 0);
    REQUIRE(actual.Minimum == 0);
    REQUIRE(actual.Maximum == 0);
    REQUIRE(actual.Mean == 0.0F);
    REQUIRE(actual.Median == 0.0F);
    REQUIRE(actual.StandardDeviation == 0.0F);
    REQUIRE(actual.Unique == 0);
    REQUIRE(actual.Modes.empty());
    REQUIRE(actual.Standardized == std::vector<float32>{0.0F, 0.0F, 0.0F});
  }

  SECTION("one tuple")
  {
    const std::vector<int16> values = {-17};
    const auto actual = RunWholeArrayCase<int16>(execute, values, WholeArrayMask::None, true);
    REQUIRE(actual.Length == 1);
    REQUIRE(actual.Minimum == -17);
    REQUIRE(actual.Maximum == -17);
    REQUIRE(actual.Mean == -17.0F);
    REQUIRE(actual.Median == -17.0F);
    REQUIRE(actual.StandardDeviation == 0.0F);
    REQUIRE(actual.Modes == std::vector<int16>{-17});
    REQUIRE(std::isnan(actual.Standardized[0]));
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array two-pass engine matches the scanline oracle", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  RunTwoPassOracleCases<int32>(*scanlineScenario);
  RunTwoPassOracleCases<uint32>(*scanlineScenario);
  RunTwoPassOracleCases<int64>(*scanlineScenario);
  RunTwoPassOracleCases<uint64>(*scanlineScenario);
  RunTwoPassOracleCases<float32>(*scanlineScenario);
  RunTwoPassOracleCases<float64>(*scanlineScenario);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array order statistics keep the sorted path for wide types", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
  const std::vector<int32> values = MakeWholeArrayInput<int32>(1000);
  const auto actual = RunWholeArrayCase<int32>(execute, values, WholeArrayMask::Bool, true);
  const TodayScanlineOracle<int32> expected = ComputeTodayScanlineOracle(values, WholeArrayMask::Bool);
  REQUIRE(actual.Median == expected.Median);
  REQUIRE(actual.Unique == expected.Unique);
  REQUIRE(actual.Modes == expected.Modes);
  // The element-order implementation computes these, so the deviation matches exactly.
  REQUIRE(actual.StandardDeviation == expected.StandardDeviation);
  const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
  REQUIRE(witness.HistogramPages == 0);
  REQUIRE(witness.MomentPages == 0);
  REQUIRE(witness.DeviationPages == 0);
  REQUIRE(witness.StandardizePages == 0);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array two-pass engine keeps NaN, signed-zero, and infinity semantics", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  const ScanSeamScope seams(k_SeamPageBytes, 2);
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
  const auto runBoth = [&](auto typeTag) {
    using T = decltype(typeTag);
    constexpr T k_NaN = std::numeric_limits<T>::quiet_NaN();
    constexpr T k_Inf = std::numeric_limits<T>::infinity();

    // NaN in the first block is ignored by min and max but propagates into sum, mean, and deviation.
    // Blocks of two tuples put -0 and +0 in different blocks; the first seen minimum (-0) is kept.
    const std::vector<T> withNaN = {k_NaN, static_cast<T>(-0.0), static_cast<T>(0.0), static_cast<T>(3), static_cast<T>(1)};
    const auto nanResult = RunWholeArrayCase<T>(execute, withNaN, WholeArrayMask::None, false);
    REQUIRE(nanResult.Length == 5);
    REQUIRE(nanResult.Minimum == static_cast<T>(0));
    REQUIRE(std::signbit(nanResult.Minimum));
    REQUIRE(nanResult.Maximum == static_cast<T>(3));
    REQUIRE(std::isnan(nanResult.Summation));
    REQUIRE(std::isnan(nanResult.Mean));
    REQUIRE(std::isnan(nanResult.StandardDeviation));

    // Only NaN values leave the sentinels in place.
    const std::vector<T> onlyNaN = {k_NaN, k_NaN, k_NaN};
    const auto onlyNaNResult = RunWholeArrayCase<T>(execute, onlyNaN, WholeArrayMask::None, false);
    REQUIRE(onlyNaNResult.Minimum == std::numeric_limits<T>::max());
    REQUIRE(onlyNaNResult.Maximum == std::numeric_limits<T>::lowest());

    // +Inf is the maximum, and its difference from the infinite mean makes the deviation NaN.
    const std::vector<T> withInf = {static_cast<T>(1), static_cast<T>(-2), k_Inf, static_cast<T>(2)};
    const auto infResult = RunWholeArrayCase<T>(execute, withInf, WholeArrayMask::None, false);
    REQUIRE(infResult.Minimum == static_cast<T>(-2));
    REQUIRE(infResult.Maximum == k_Inf);
    REQUIRE(infResult.Summation == std::numeric_limits<float32>::infinity());
    REQUIRE(infResult.Mean == std::numeric_limits<float32>::infinity());
    REQUIRE(std::isnan(infResult.StandardDeviation));

    // A +0 before -0 keeps +0 as the minimum; +Inf alone never beats the max() sentinel.
    const std::vector<T> positiveZeroFirst = {static_cast<T>(0.0), static_cast<T>(-0.0), k_Inf};
    const auto zeroResult = RunWholeArrayCase<T>(execute, positiveZeroFirst, WholeArrayMask::None, false);
    REQUIRE(zeroResult.Minimum == static_cast<T>(0));
    REQUIRE_FALSE(std::signbit(zeroResult.Minimum));
    REQUIRE(zeroResult.Maximum == k_Inf);

    for(const auto* values : {&withNaN, &onlyNaN, &withInf, &positiveZeroFirst})
    {
      const TodayScanlineOracle<T> expected = ComputeTodayScanlineOracle(*values, WholeArrayMask::None);
      const auto actual = RunWholeArrayCase<T>(execute, *values, WholeArrayMask::None, false);
      REQUIRE(actual.Length == expected.Length);
      // Signed zeros compare by value.
      REQUIRE(actual.Minimum == expected.Minimum);
      REQUIRE(actual.Maximum == expected.Maximum);
      REQUIRE(WithinRegroupedTolerance(actual.Summation, expected.Summation, 0.0));
      REQUIRE(WithinRegroupedTolerance(actual.Mean, expected.Mean, 0.0));
      REQUIRE(WithinRegroupedTolerance(actual.StandardDeviation, expected.StandardDeviation, 0.0));
    }
  };
  runBoth(float32{});
  runBoth(float64{});
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array two-pass engine agrees with the Direct path", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto compareType = [](auto typeTag) {
    using T = decltype(typeTag);
    for(const WholeArrayMask maskKind : {WholeArrayMask::None, WholeArrayMask::UInt8})
    {
      CAPTURE(std::string(DataTypeToString(GetDataType<T>()).view()), static_cast<int32>(maskKind));
      const std::vector<T> values = MakeWholeArrayInput<T>(4099);
      const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
      UnitTest::AlgorithmTestScope directScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
      const auto executeDirect = [&directScope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) {
        return directScope.executeFilter(filter, dataStructure, args);
      };
      const WholeArrayResult<T> direct = RunWholeArrayCase<T>(executeDirect, values, maskKind, false);
      UnitTest::AlgorithmTestScope scanlineScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
      const auto executeScanline = [&scanlineScope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) {
        return scanlineScope.executeFilter(filter, dataStructure, args);
      };
      const WholeArrayResult<T> scanline = RunWholeArrayCase<T>(executeScanline, values, maskKind, false);
      REQUIRE(GetComputeArrayStatisticsScanWitnessForTesting().MomentPages > 0);

      const TodayScanlineOracle<T> reference = ComputeTodayScanlineOracle(values, maskKind);
      const float64 sumBound = static_cast<float64>(reference.Length) * std::numeric_limits<float64>::epsilon() * reference.AbsoluteSum;
      REQUIRE(scanline.Length == direct.Length);
      REQUIRE(scanline.Minimum == direct.Minimum);
      REQUIRE(scanline.Maximum == direct.Maximum);
      CHECK(WithinRegroupedTolerance(scanline.Summation, direct.Summation, sumBound));
      CHECK(WithinRegroupedTolerance(scanline.Mean, direct.Mean, sumBound / static_cast<float64>(reference.Length) + std::fabs(static_cast<float64>(direct.Mean)) * 1.0e-6));
      REQUIRE(scanline.StandardDeviation == Approx(direct.StandardDeviation).epsilon(1.0e-6));
      for(usize index = 0; index < values.size(); ++index)
      {
        REQUIRE(scanline.Standardized[index] == Approx(direct.Standardized[index]).epsilon(1.0e-5).margin(1.0e-6));
      }
    }
  };
  compareType(int32{});
  compareType(uint32{});
  compareType(int64{});
  compareType(uint64{});
  compareType(float32{});
  compareType(float64{});
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array engines do not depend on the thread count or page size", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
#ifdef SIMPLNX_ENABLE_MULTICORE
  // One thread against the default limit, which permits every core of the machine.
  constexpr usize k_SerialLimit = 1;
#else
  constexpr usize k_SerialLimit = 0;
#endif
  const auto compareType = [&scanlineScenario](auto typeTag, bool orderStatistics) {
    using T = decltype(typeTag);
    CAPTURE(std::string(DataTypeToString(GetDataType<T>()).view()));
    const std::vector<T> values = MakeWholeArrayInput<T>(20011);
    const WholeArrayResult<T> serial = RunScanlineWithLimits<T>(*scanlineScenario, values, orderStatistics, k_SeamPageBytes, k_SerialLimit);
    const WholeArrayResult<T> parallel = RunScanlineWithLimits<T>(*scanlineScenario, values, orderStatistics, k_SeamPageBytes, 0);
    RequireBitIdentical(serial, parallel);
    // A four-times larger page keeps the same global block grid.
    const WholeArrayResult<T> largerPages = RunScanlineWithLimits<T>(*scanlineScenario, values, orderStatistics, 4 * k_SeamPageBytes, 0);
    RequireBitIdentical(serial, largerPages);
  };
  compareType(uint16{}, true);
  compareType(int8{}, true);
  compareType(int32{}, false);
  compareType(uint64{}, false);
  compareType(float32{}, false);
  compareType(float64{}, false);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array engines on genuine out-of-core stores", "[SimplnxCore][ComputeArrayStatisticsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  if(!DataStoreUtilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"))
  {
    SUCCEED("No out-of-core store format is registered.");
    return;
  }
  UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  const auto execute = [](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return filter.execute(dataStructure, args); };
  // Several KiB pages over a 200000-tuple input read many chunk-sized pieces with a partial last page.
  constexpr usize k_TupleCount = 200003;
  constexpr usize k_PageBytes = 64 * 1024;
  constexpr usize k_BlockTuples = 1024;

  SECTION("uint16 histogram engine")
  {
    const std::vector<uint16> values = MakeWholeArrayInput<uint16>(k_TupleCount);
    const ScanSeamScope seams(k_PageBytes, k_BlockTuples);
    const auto actual = RunWholeArrayCase<uint16>(execute, values, WholeArrayMask::UInt8, true, true);
    RequireHistogramResultMatchesOracle(values, WholeArrayMask::UInt8, actual);
    const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
    REQUIRE(witness.HistogramPages > 1);
    REQUIRE(witness.StandardizePages > 1);
  }

  SECTION("int32 two-pass engine")
  {
    const std::vector<int32> values = MakeWholeArrayInput<int32>(k_TupleCount);
    const ScanSeamScope seams(k_PageBytes, k_BlockTuples);
    const auto actual = RunWholeArrayCase<int32>(execute, values, WholeArrayMask::None, false, true);
    RequireTwoPassResultMatchesOracle(values, WholeArrayMask::None, actual);
    const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
    REQUIRE(witness.MomentPages > 1);
    REQUIRE(witness.DeviationPages == witness.MomentPages);
    REQUIRE(witness.StandardizePages > 1);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array engines count non-canonical Boolean bytes once", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  const auto execute = [&scope](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return scope.executeFilter(filter, dataStructure, args); };
  RequireNonCanonicalBoolBytesCountOnce(execute, 4099, false);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array engines count non-canonical Boolean bytes once on genuine out-of-core stores",
          "[SimplnxCore][ComputeArrayStatisticsFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  if(!DataStoreUtilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"))
  {
    SUCCEED("No out-of-core store format is registered.");
    return;
  }
  UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  const ScanSeamScope seams(64 * 1024, 1024);
  const auto execute = [](ComputeArrayStatisticsFilter& filter, DataStructure& dataStructure, const Arguments& args) { return filter.execute(dataStructure, args); };
  RequireNonCanonicalBoolBytesCountOnce(execute, 200003, true);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array masked Standardize keeps the current value of unselected tuples", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline engine contract is omitted by the InCoreOnly selection.");
    return;
  }
  // The filter creates a zero-filled output, so a zero-filled piece buffer could hide a missing read-back.
  // The algorithm runs directly on an output that holds a nonzero sentinel instead.
  constexpr float32 k_Sentinel = 42.0F;
  constexpr usize k_TupleCount = 4099;
  const auto runType = [&scanlineScenario, k_Sentinel](auto typeTag, WholeArrayMask maskKind) {
    using T = decltype(typeTag);
    CAPTURE(std::string(DataTypeToString(GetDataType<T>()).view()), static_cast<int32>(maskKind));
    const std::vector<T> values = MakeWholeArrayInput<T>(k_TupleCount);
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroup != nullptr);
    REQUIRE(output != nullptr);
    auto* input = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, "Values", {k_TupleCount}, {1}, inputGroup->getId());
    auto* mean = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Mean", {1}, {1}, output->getId());
    auto* deviation = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "StandardDeviation", {1}, {1}, output->getId());
    auto* standardized = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Standardized", {k_TupleCount}, {1}, inputGroup->getId());
    REQUIRE(input != nullptr);
    REQUIRE(mean != nullptr);
    REQUIRE(deviation != nullptr);
    REQUIRE(standardized != nullptr);
    WriteTestValues(input->getDataStoreRef(), values);
    standardized->fill(k_Sentinel);
    if(maskKind == WholeArrayMask::Bool)
    {
      auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
      REQUIRE(mask != nullptr);
      std::vector<bool> maskValues(k_TupleCount);
      for(usize index = 0; index < k_TupleCount; ++index)
      {
        maskValues[index] = IsSelectedByTestMask(maskKind, index);
      }
      WriteTestValues(mask->getDataStoreRef(), maskValues);
    }
    else
    {
      auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
      REQUIRE(mask != nullptr);
      std::vector<uint8> maskValues(k_TupleCount);
      for(usize index = 0; index < k_TupleCount; ++index)
      {
        maskValues[index] = IsSelectedByTestMask(maskKind, index) ? 1 : 0;
      }
      WriteTestValues(mask->getDataStoreRef(), maskValues);
    }

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindMean = true;
    inputValues.FindStdDeviation = true;
    inputValues.StandardizeData = true;
    inputValues.UseMask = true;
    inputValues.MaskArrayPath = DataPath({"Input", "Mask"});
    const std::atomic_bool shouldCancel = false;
    const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
    UnitTest::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    // Several pieces make the piece buffer hold values of an earlier piece.
    REQUIRE(GetComputeArrayStatisticsScanWitnessForTesting().StandardizePages > 1);

    const float32 actualMean = (*mean)[0];
    const float32 actualDeviation = (*deviation)[0];
    const TodayScanlineOracle<T> expected = ComputeTodayScanlineOracle(values, maskKind);
    if constexpr(std::is_integral_v<T>)
    {
      REQUIRE(actualMean == expected.Mean);
    }
    REQUIRE(actualDeviation == Approx(expected.StandardDeviation).epsilon(1.0e-6));
    std::vector<float32> actual(k_TupleCount);
    auto readResult = standardized->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    usize unselectedCount = 0;
    for(usize index = 0; index < k_TupleCount; ++index)
    {
      if(IsSelectedByTestMask(maskKind, index))
      {
        REQUIRE(actual[index] == (static_cast<float32>(values[index]) - actualMean) / actualDeviation);
      }
      else
      {
        REQUIRE(actual[index] == k_Sentinel);
        ++unselectedCount;
      }
    }
    REQUIRE(unselectedCount > 0);
  };
  runType(uint16{}, WholeArrayMask::Bool);
  runType(uint16{}, WholeArrayMask::UInt8);
  runType(int32{}, WholeArrayMask::Bool);
  runType(float32{}, WholeArrayMask::UInt8);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: whole-array deviation-pass cancellation preserves unwritten outputs", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  constexpr usize k_TupleCount = 4099;
  const usize pagesPerPass = ExpectedReductionPages(k_TupleCount, sizeof(int32));
  std::atomic_bool shouldCancel = false;
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  auto* output = AttributeMatrix::Create(dataStructure, "Statistics", {1});
  REQUIRE(inputGroup != nullptr);
  REQUIRE(output != nullptr);
  // The first read of the deviation pass sets the cancellation flag.
  auto inputStore = std::make_shared<StatisticsCancelAfterReadsDataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{3}, shouldCancel, pagesPerPass + 1);
  auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
  auto* length = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, output->getId());
  auto* deviation = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "StandardDeviation", {1}, {1}, output->getId());
  REQUIRE(values != nullptr);
  REQUIRE(length != nullptr);
  REQUIRE(deviation != nullptr);
  length->fill(94);
  deviation->fill(95.0F);
  ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
  inputValues.FindLength = true;
  inputValues.FindStdDeviation = true;
  const ScanSeamScope seams(k_SeamPageBytes, k_SeamBlockTuples);
  UnitTest::AlgorithmTestScope scope(*scanlineScenario);
  const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  REQUIRE(shouldCancel.load());
  REQUIRE(inputStore->readCount() == pagesPerPass + 1);
  const ComputeArrayStatisticsScanWitness witness = GetComputeArrayStatisticsScanWitnessForTesting();
  REQUIRE(witness.MomentPages == pagesPerPass);
  REQUIRE(witness.DeviationPages == 0);
  REQUIRE((*length)[0] == 94);
  REQUIRE((*deviation)[0] == 95.0F);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {11}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 45;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 9;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 1;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {11}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = false;
  maskDataStore[7] = true;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 11);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto modeVals = (*modeArray).getList(0);
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];
    auto numUnique = (*numUniqueValuesArray)[0];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 45);
    REQUIRE(modeVals.size() == 1);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(meanVal - 14.3333f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 10.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 13.02f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal - 129.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 8);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = std::ceil(standardizeDataStore[0] * 100.0f) / 100.0f;
    auto stand1 = std::ceil(standardizeDataStore[1] * 100.0f) / 100.0f;
    auto stand2 = standardizeDataStore[2]; //
    auto stand3 = std::ceil(standardizeDataStore[3] * 100.0f) / 100.0f;
    auto stand4 = std::ceil(standardizeDataStore[4] * 100.0f) / 100.0f;
    auto stand5 = std::ceil(standardizeDataStore[5] * 100.0f) / 100.0f;
    auto stand6 = standardizeDataStore[6]; //
    auto stand7 = std::ceil(standardizeDataStore[7] * 100.0f) / 100.0f;
    auto stand8 = std::ceil(standardizeDataStore[8] * 100.0f) / 100.0f;
    auto stand9 = std::ceil(standardizeDataStore[9] * 100.0f) / 100.0f;
    REQUIRE(std::fabs(stand0 - -1.01999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.43999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand2 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand3 - 2.35999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -0.70999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.12999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand6 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand7 - .58999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - -.4f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -.33f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 3);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 3);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 3);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 3);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 3);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 3);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 3);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 3);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 3);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto lengthVal3 = (*lengthArray)[2];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto minVal3 = (*minArray)[2];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto maxVal3 = (*maxArray)[2];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto meanVal3 = (*meanArray)[2];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto medianVal3 = (*medianArray)[2];
    auto modes0 = (*modeArray).getList(0);
    auto modes1 = (*modeArray).getList(1);
    auto modes2 = (*modeArray).getList(2);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto stdVal3 = (*stdArray)[2];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto sumVal3 = (*sumArray)[2];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto numUnique3 = (*numUniqueValuesArray)[2];

    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 3);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - -1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Ignore 0", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Ignore feature 0
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 2);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 2);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 2);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 2);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 2);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 2);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 2);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 2);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 2);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 2);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto modes1 = (*modeArray).getList(0);
    auto modes2 = (*modeArray).getList(1);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Shrink to Fit", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(2ULL)); // Shrink To Fit
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 3);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 3);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 3);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 3);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 3);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 3);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 3);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 3);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 3);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 3);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto lengthVal3 = (*lengthArray)[2];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto minVal3 = (*minArray)[2];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto maxVal3 = (*maxArray)[2];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto meanVal3 = (*meanArray)[2];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto medianVal3 = (*medianArray)[2];
    auto modes0 = (*modeArray).getList(0);
    auto modes1 = (*modeArray).getList(1);
    auto modes2 = (*modeArray).getList(2);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto stdVal3 = (*stdArray)[2];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto sumVal3 = (*sumArray)[2];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto numUnique3 = (*numUniqueValuesArray)[2];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];
    auto mapping3 = (*featureIdMappingArray)[2];

    REQUIRE(mapping1 == 0);
    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);
    REQUIRE(mapping2 == 1);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 3);
    REQUIRE(mapping3 == 2);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - -1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Padded Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";
  const std::string featureHasData = "FeatureHasData";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(3ULL)); // Padded Custom Range
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureHasDataArrayName_Key, std::make_any<std::string>(featureHasData));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32Parameter::ValueType>({1, 4}));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 4);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 4);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 4);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 4);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 4);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 4);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 4);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 4);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 4);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 4);
    auto* featureHasDataArray = dataStructure.getDataAs<BoolArray>(statsDataPath.createChildPath(featureHasData));
    REQUIRE(featureHasDataArray != nullptr);
    REQUIRE(featureHasDataArray->getNumberOfTuples() == 4);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto modes1 = (*modeArray).getList(0);
    auto modes2 = (*modeArray).getList(1);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];
    auto mapping3 = (*featureIdMappingArray)[2];
    auto mapping4 = (*featureIdMappingArray)[3];
    auto featureHasData1 = (*featureHasDataArray)[0];
    auto featureHasData2 = (*featureHasDataArray)[1];
    auto featureHasData3 = (*featureHasDataArray)[2];
    auto featureHasData4 = (*featureHasDataArray)[3];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 2);
    REQUIRE(mapping3 == 3);
    REQUIRE(mapping4 == 4);
    REQUIRE(featureHasData1);
    REQUIRE(featureHasData2);
    REQUIRE(!featureHasData3);
    REQUIRE(!featureHasData4);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the configured filter.
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(4ULL)); // Custom Range
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32Parameter::ValueType>({2, 4}));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 1);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 1);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 1);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 1);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 1);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 1);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 1);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 1);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 1);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 1);

    auto lengthVal1 = (*lengthArray)[0];
    auto minVal1 = (*minArray)[0];
    auto maxVal1 = (*maxArray)[0];
    auto meanVal1 = (*meanArray)[0];
    auto medianVal1 = (*medianArray)[0];
    auto modes1 = (*modeArray).getList(0);
    auto stdVal1 = (*stdArray)[0];
    auto sumVal1 = (*sumArray)[0];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto mapping1 = (*featureIdMappingArray)[0];

    REQUIRE(mapping1 == 2);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 10);
    REQUIRE(maxVal1 == 22);
    REQUIRE(std::fabs(meanVal1 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 2);
    REQUIRE(VectorContains(modes1, 10) == true);
    REQUIRE(VectorContains(modes1, 22) == true);
    REQUIRE(std::fabs(stdVal1 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(stand0 == 0.0f);
    REQUIRE(stand1 == 0.0f);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(stand3 == 0.0f);
    REQUIRE(stand4 == 0.0f);
    REQUIRE(stand5 == 0.0f);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(stand7 == 0.0f);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeArrayStatisticsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeArrayStatisticsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeArrayStatisticsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeArrayStatisticsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindLength_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMin_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMax_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMean_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMedian_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindSummation_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_StandardizeData_Key) == true);
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_LengthArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MeanArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MedianArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_SummationArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key) == "TestName");
    }
  }
}

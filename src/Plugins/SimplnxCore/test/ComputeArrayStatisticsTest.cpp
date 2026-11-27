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
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <optional>
#include <system_error>
#include <vector>

using namespace nx::core;
using namespace nx::core::constants;
namespace fs = std::filesystem;

namespace
{
std::optional<unit_test::AlgorithmTestScenario> SelectScanlineInMemoryScenario()
{
  const auto scenarios = unit_test::SelectAlgorithmTestScenariosForInMemoryStores();
  const auto iter = std::find(scenarios.cbegin(), scenarios.cend(), unit_test::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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

  Result<> copyIntoBuffer(usize /*unused*/, nonstd::span<T> /*unused*/) const override
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

  Result<> copyFromBuffer(usize /*unused*/, nonstd::span<const T> /*unused*/) override
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
  inputValues.range_type = to_underlying(ComputeArrayStatistics::FeatureIdRangeControls::None);
  inputValues.range = {0, -1};
  inputValues.selected_array_path = inputPath;
  inputValues.destination_attribute_matrix = destinationPath;
  inputValues.feature_has_data_array_name = destinationPath.createChildPath("FeatureHasData");
  inputValues.length_array_name = destinationPath.createChildPath("Length");
  inputValues.minimum_array_name = destinationPath.createChildPath("Minimum");
  inputValues.maximum_array_name = destinationPath.createChildPath("Maximum");
  inputValues.mean_array_name = destinationPath.createChildPath("Mean");
  inputValues.median_array_name = destinationPath.createChildPath("Median");
  inputValues.mode_array_name = destinationPath.createChildPath("Mode");
  inputValues.std_deviation_array_name = destinationPath.createChildPath("StandardDeviation");
  inputValues.summation_array_name = destinationPath.createChildPath("Summation");
  inputValues.standardized_array_name = inputPath.replaceName("Standardized");
  inputValues.num_unique_values_name = destinationPath.createChildPath("NumUniqueValues");
  inputValues.feature_id_map_array_path = destinationPath.createChildPath("FeatureIdMap");
  return inputValues;
}

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
void RunCompactNumericStatistics(unit_test::AlgorithmTestScope& scope)
{
  DataStructure dataStructure;
  auto* valuesPtr = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, "Values", {4}, {1});
  REQUIRE(valuesPtr != nullptr);
  const std::array<T, 4> input = {static_cast<T>(1), static_cast<T>(2), static_cast<T>(2), static_cast<T>(5)};
  auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == 4);
  REQUIRE(dataStructure.getDataRefAs<DataArray<T>>(DataPath({"Statistics", "Minimum"}))[0] == static_cast<T>(1));
  REQUIRE(dataStructure.getDataRefAs<DataArray<T>>(DataPath({"Statistics", "Maximum"}))[0] == static_cast<T>(5));
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"}))[0] == Approx(10.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[0] == Approx(2.5F));
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == Approx(2.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[0] == Approx(1.5F));
  REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 3);
  if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  {
    REQUIRE(dataStructure.getDataRefAs<NeighborList<T>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<T>{static_cast<T>(2)});
  }
}

template <>
void RunCompactNumericStatistics<bool>(unit_test::AlgorithmTestScope& scope)
{
  DataStructure dataStructure;
  auto* valuesPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Values", {4}, {1});
  REQUIRE(valuesPtr != nullptr);
  const std::array<bool, 4> input = {false, true, true, true};
  auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == 4);
  REQUIRE_FALSE(dataStructure.getDataRefAs<BoolArrayType>(DataPath({"Statistics", "Minimum"}))[0]);
  REQUIRE(dataStructure.getDataRefAs<BoolArrayType>(DataPath({"Statistics", "Maximum"}))[0]);
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"}))[0] == 3.0F);
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[0] == 0.75F);
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == 1.0F);
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[0] == Approx(0.4330127F));
  REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 2);
}

struct StatisticsParityResult
{
  std::vector<uint64> length;
  std::vector<int32> minimum;
  std::vector<int32> maximum;
  std::vector<float32> mean;
  std::vector<float32> median;
  std::vector<float32> standard_deviation;
  std::vector<float32> summation;
  std::vector<int32> unique;
  std::vector<uint8> feature_has_data;
  std::vector<std::vector<int32>> modes;
  std::vector<float32> standardized;
};

StatisticsParityResult RunStatisticsParityCase(unit_test::AlgorithmTestScope& scope, bool computeByIndex, int32 maskKind)
{
  DataStructure dataStructure;
  auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroupPtr != nullptr);
  constexpr usize k_TupleCount = 8;
  auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {k_TupleCount}, {1}, inputGroupPtr->getId());
  auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {k_TupleCount}, {1}, inputGroupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(featureIdsPtr != nullptr);
  const std::array<int32, k_TupleCount> input = {4, 1, 4, 2, 8, 2, 6, 1};
  const std::array<int32, k_TupleCount> ids = {0, 0, 1, 1, 2, 2, 4, 4};
  const std::array<bool, k_TupleCount> boolMaskValues = {true, true, true, false, true, true, false, true};
  const std::array<uint8, k_TupleCount> uint8MaskValues = {1, 1, 1, 0, 1, 1, 0, 1};
  auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, ids);
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
  std::optional<DataPath> maskPath;
  if(maskKind == 1)
  {
    auto* maskPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroupPtr->getId());
    REQUIRE(maskPtr != nullptr);
    auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, boolMaskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    maskPath = DataPath({"Input", "Mask"});
  }
  else if(maskKind == 2)
  {
    auto* maskPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroupPtr->getId());
    REQUIRE(maskPtr != nullptr);
    auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, uint8MaskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    maskPath = DataPath({"Input", "Mask"});
  }

  ComputeArrayStatisticsFilter filter;
  Arguments args =
      CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), computeByIndex ? std::optional<DataPath>{DataPath({"Input", "FeatureIds"})} : std::nullopt, maskPath);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath statisticsPath({"Statistics"});
  const usize groupCount = dataStructure.getDataRefAs<UInt64ArrayType>(statisticsPath.createChildPath("Length")).getNumberOfTuples();
  StatisticsParityResult result;
  result.length.resize(groupCount);
  result.minimum.resize(groupCount);
  result.maximum.resize(groupCount);
  result.mean.resize(groupCount);
  result.median.resize(groupCount);
  result.standard_deviation.resize(groupCount);
  result.summation.resize(groupCount);
  result.unique.resize(groupCount);
  result.modes.resize(groupCount);
  result.standardized.resize(k_TupleCount);
  auto lengthReadResult = dataStructure.getDataRefAs<UInt64ArrayType>(statisticsPath.createChildPath("Length")).getDataStoreRef().copyIntoBuffer(0, result.length);
  SIMPLNX_RESULT_REQUIRE_VALID(lengthReadResult);
  auto minimumReadResult = dataStructure.getDataRefAs<Int32ArrayType>(statisticsPath.createChildPath("Minimum")).getDataStoreRef().copyIntoBuffer(0, result.minimum);
  SIMPLNX_RESULT_REQUIRE_VALID(minimumReadResult);
  auto maximumReadResult = dataStructure.getDataRefAs<Int32ArrayType>(statisticsPath.createChildPath("Maximum")).getDataStoreRef().copyIntoBuffer(0, result.maximum);
  SIMPLNX_RESULT_REQUIRE_VALID(maximumReadResult);
  auto meanReadResult = dataStructure.getDataRefAs<Float32ArrayType>(statisticsPath.createChildPath("Mean")).getDataStoreRef().copyIntoBuffer(0, result.mean);
  SIMPLNX_RESULT_REQUIRE_VALID(meanReadResult);
  auto medianReadResult = dataStructure.getDataRefAs<Float32ArrayType>(statisticsPath.createChildPath("Median")).getDataStoreRef().copyIntoBuffer(0, result.median);
  SIMPLNX_RESULT_REQUIRE_VALID(medianReadResult);
  auto standardDeviationReadResult = dataStructure.getDataRefAs<Float32ArrayType>(statisticsPath.createChildPath("StandardDeviation")).getDataStoreRef().copyIntoBuffer(0, result.standard_deviation);
  SIMPLNX_RESULT_REQUIRE_VALID(standardDeviationReadResult);
  auto summationReadResult = dataStructure.getDataRefAs<Float32ArrayType>(statisticsPath.createChildPath("Summation")).getDataStoreRef().copyIntoBuffer(0, result.summation);
  SIMPLNX_RESULT_REQUIRE_VALID(summationReadResult);
  auto numUniqueValuesReadResult = dataStructure.getDataRefAs<Int32ArrayType>(statisticsPath.createChildPath("NumUniqueValues")).getDataStoreRef().copyIntoBuffer(0, result.unique);
  SIMPLNX_RESULT_REQUIRE_VALID(numUniqueValuesReadResult);
  auto standardizedReadResult = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, result.standardized);
  SIMPLNX_RESULT_REQUIRE_VALID(standardizedReadResult);
  if(computeByIndex)
  {
    result.feature_has_data.resize(groupCount);
    auto featureHasData = std::make_unique<bool[]>(groupCount);
    auto featureHasDataReadResult =
        dataStructure.getDataRefAs<BoolArrayType>(statisticsPath.createChildPath("FeatureHasData")).getDataStoreRef().copyIntoBuffer(0, nonstd::span<bool>(featureHasData.get(), groupCount));
    SIMPLNX_RESULT_REQUIRE_VALID(featureHasDataReadResult);
    for(usize group = 0; group < groupCount; ++group)
    {
      result.feature_has_data[group] = featureHasData[group] ? 1 : 0;
    }
  }
  const auto& mode = dataStructure.getDataRefAs<NeighborList<int32>>(statisticsPath.createChildPath("Mode"));
  for(usize group = 0; group < groupCount; ++group)
  {
    result.modes[group] = mode.getList(static_cast<int32>(group));
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

  Result<> append(uint64, nonstd::span<const std::byte>, const std::atomic_bool&, const ExternalSortProgressCallbackType&) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected statistics external-sort append failure");
  }

  Result<> finish(const std::atomic_bool&, const ExternalSortProgressCallbackType&) override
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
  unit_test::LoadPlugins();
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
  const auto scenarios = unit_test::SelectAlgorithmTestScenariosForInMemoryStores();
  for(const auto scenario : scenarios)
  {
    CAPTURE(scenario);
    unit_test::AlgorithmTestScope scope(scenario);
    const auto actual = RunStatisticsParityCase(scope, computeByIndex, maskKind);
    // Reduce the eight known input values independently. This reference uses
    // sorted small vectors and population variance, without production helpers.
    const std::array<int32, 8> input = {4, 1, 4, 2, 8, 2, 6, 1};
    const std::array<usize, 8> groups = {0, 0, 1, 1, 2, 2, 4, 4};
    const usize groupCount = computeByIndex ? 5 : 1;
    REQUIRE(actual.length.size() == groupCount);
    REQUIRE(actual.minimum.size() == groupCount);
    REQUIRE(actual.maximum.size() == groupCount);
    REQUIRE(actual.mean.size() == groupCount);
    REQUIRE(actual.median.size() == groupCount);
    REQUIRE(actual.standard_deviation.size() == groupCount);
    REQUIRE(actual.summation.size() == groupCount);
    REQUIRE(actual.unique.size() == groupCount);
    REQUIRE(actual.modes.size() == groupCount);
    REQUIRE(actual.standardized.size() == input.size());
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
      CHECK(actual.length[groupIdx] == selected.size());
      if(computeByIndex)
      {
        REQUIRE(actual.feature_has_data.size() == groupCount);
        CHECK(actual.feature_has_data[groupIdx] == (selected.empty() ? 0 : 1));
      }
      if(selected.empty())
      {
        CHECK(actual.minimum[groupIdx] == std::numeric_limits<int32>::max());
        CHECK(actual.maximum[groupIdx] == std::numeric_limits<int32>::min());
        CHECK(actual.mean[groupIdx] == 0.0F);
        CHECK(actual.median[groupIdx] == 0.0F);
        CHECK(actual.standard_deviation[groupIdx] == 0.0F);
        CHECK(actual.summation[groupIdx] == 0.0F);
        CHECK(actual.unique[groupIdx] == 0);
        CHECK(actual.modes[groupIdx].empty());
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
      CHECK(actual.minimum[groupIdx] == selected.front());
      CHECK(actual.maximum[groupIdx] == selected.back());
      CHECK(actual.summation[groupIdx] == Approx(sum));
      CHECK(actual.mean[groupIdx] == Approx(mean));
      CHECK(actual.standard_deviation[groupIdx] == Approx(deviation).margin(1.0E-5));
      CHECK(actual.median[groupIdx] == Approx((selected[(selected.size() - 1) / 2] + selected[selected.size() / 2]) / 2.0));
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
      CHECK(actual.unique[groupIdx] == uniqueCount);
      CHECK(actual.modes[groupIdx] == modes);
      for(usize tupleIdx = 0; tupleIdx < input.size(); ++tupleIdx)
      {
        if(computeByIndex && groups[tupleIdx] != groupIdx)
        {
          continue;
        }
        if(maskKind != 0 && (tupleIdx == 3 || tupleIdx == 6))
        {
          CHECK(actual.standardized[tupleIdx] == 0.0F);
        }
        else if(deviation == 0.0)
        {
          CHECK(std::isnan(actual.standardized[tupleIdx]));
        }
        else
        {
          CHECK(actual.standardized[tupleIdx] == Approx((input[tupleIdx] - mean) / deviation).margin(1.0E-5));
        }
      }
    }
    if(hasExpectedResult)
    {
      REQUIRE(actual.length == expected.length);
      REQUIRE(actual.minimum == expected.minimum);
      REQUIRE(actual.maximum == expected.maximum);
      REQUIRE(actual.unique == expected.unique);
      REQUIRE(actual.feature_has_data == expected.feature_has_data);
      REQUIRE(actual.modes == expected.modes);
      requireFloatParity(actual.mean, expected.mean);
      requireFloatParity(actual.median, expected.median);
      requireFloatParity(actual.standard_deviation, expected.standard_deviation);
      requireFloatParity(actual.summation, expected.summation);
      requireFloatParity(actual.standardized, expected.standardized);
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
  unit_test::LoadPlugins();
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
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputPtr = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    auto inputStore = std::make_shared<StatisticsFailingReadDataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1}, k_ReadError);
    auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", inputStore, inputGroupPtr->getId());
    auto* lengthPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, outputPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(lengthPtr != nullptr);

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.find_length = true;
    const std::atomic_bool shouldCancel = false;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_ReadError);
  }

  SECTION("numeric output bulk write")
  {
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputPtr = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroupPtr->getId());
    auto lengthStore = std::make_shared<StatisticsFailingWriteDataStore<uint64>>(ShapeType{1}, ShapeType{1}, std::optional<uint64>{0}, k_WriteError);
    auto* lengthPtr = UInt64ArrayType::Create(dataStructure, "Length", lengthStore, outputPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(lengthPtr != nullptr);
    const std::array<int32, 4> input = {1, 2, 3, 4};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.find_length = true;
    const std::atomic_bool shouldCancel = false;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: a damaged out-of-core backing file surfaces as a filter error", "[SimplnxCore][ComputeArrayStatisticsFilter][.OocStoreContract]")
{
  unit_test::LoadPlugins();
  auto& ioCollection = data_store_utilities::GetIOCollection();
  if(!ioCollection.hasDataStoreCreationFunction("HDF5-OOC"))
  {
    return;
  }

  unit_test::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_TupleCount = (64ULL * 1024ULL * 1024ULL) / sizeof(int32);
  const DataPath inputPath({"DamagedInput"});
  const DataPath statisticsPath({"Statistics"});

  DataStructure dataStructure;
  auto inputStore = data_store_utilities::CreateDataStore<int32>(dataStructure, inputPath, {k_TupleCount}, {1});
  REQUIRE(inputStore != nullptr);
  auto* inputArrayPtr = Int32ArrayType::Create(dataStructure, inputPath.getTargetName(), inputStore);
  REQUIRE(inputArrayPtr != nullptr);
  REQUIRE(inputArrayPtr->getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);

  const auto recoveryMetadata = inputArrayPtr->getDataStoreRef().getRecoveryMetadata();
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
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }

  SECTION("entry cancellation")
  {
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputPtr = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroupPtr->getId());
    auto* lengthPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, outputPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(lengthPtr != nullptr);
    lengthPtr->fill(91);
    const std::atomic_bool shouldCancel = true;
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.find_length = true;
    const auto executeResult = ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)();
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE((*lengthPtr)[0] == 91);
  }

  SECTION("mid-pass cancellation")
  {
    constexpr usize k_TupleCount = 65537;
    std::atomic_bool shouldCancel = false;
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputPtr = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    auto inputStore = std::make_shared<StatisticsCancelAfterReadDataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{1}, shouldCancel);
    auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", inputStore, inputGroupPtr->getId());
    auto* lengthPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Length", {1}, {1}, outputPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(lengthPtr != nullptr);
    lengthPtr->fill(92);
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.find_length = true;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(shouldCancel.load());
    REQUIRE((*lengthPtr)[0] == 92);
  }

  SECTION("between output writes")
  {
    std::atomic_bool shouldCancel = false;
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputPtr = AttributeMatrix::Create(dataStructure, "Statistics", {1});
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroupPtr->getId());
    auto lengthStore = std::make_shared<StatisticsCancelAfterWriteDataStore<uint64>>(ShapeType{1}, ShapeType{1}, std::optional<uint64>{0}, shouldCancel);
    auto* lengthPtr = UInt64ArrayType::Create(dataStructure, "Length", lengthStore, outputPtr->getId());
    auto* minimumPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Minimum", {1}, {1}, outputPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(lengthPtr != nullptr);
    REQUIRE(minimumPtr != nullptr);
    const std::array<int32, 4> input = {4, 3, 2, 1};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    minimumPtr->fill(93);
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.find_length = true;
    inputValues.find_min = true;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(shouldCancel.load());
    REQUIRE((*lengthPtr)[0] == 4);
    REQUIRE((*minimumPtr)[0] == 93);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Unique-only without Length", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(groupPtr != nullptr);
  auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {5}, {1}, groupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  const std::array<int32, 5> input = {4, 4, 2, 7, 2};
  auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  scope.requireExpectedStore(*valuesPtr);
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"})));
  const auto& unique = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}));
  REQUIRE(unique.getDataStoreRef()[0] == 3);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: bounded exact fallback without external sort", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  if(data_store_utilities::GetIOCollection().hasExternalSortCapability())
  {
    SUCCEED("An external-sort provider is registered; the no-provider fallback does not apply.");
    return;
  }
  unit_test::AlgorithmTestScope scope(*scanlineScenario);
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(groupPtr != nullptr);
  auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {6}, {1}, groupPtr->getId());
  auto* maskPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {6}, {1}, groupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  const std::array<int32, 6> input = {5, 1, 5, 3, 3, 9};
  const std::array<bool, 6> maskValues = {true, true, true, true, true, true};
  auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
  auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, maskValues);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == 6);
  REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Minimum"}))[0] == 1);
  REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Maximum"}))[0] == 9);
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[0] == Approx(26.0F / 6.0F));
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == 4.0F);
  REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{3, 5});
  REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"}))[0] == 26.0F);
  REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 4);

  constexpr usize k_UniqueCount = 257;
  DataStructure uniqueData;
  auto* uniqueInputGroupPtr = DataGroup::Create(uniqueData, "Input");
  REQUIRE(uniqueInputGroupPtr != nullptr);
  auto* uniqueInputPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(uniqueData, "Values", {k_UniqueCount}, {1}, uniqueInputGroupPtr->getId());
  REQUIRE(uniqueInputPtr != nullptr);
  std::array<int32, k_UniqueCount> descending = {};
  std::vector<int32> expectedModes(k_UniqueCount);
  for(usize index = 0; index < k_UniqueCount; ++index)
  {
    descending[index] = static_cast<int32>(k_UniqueCount - index - 1);
    expectedModes[index] = static_cast<int32>(index);
  }
  auto uniqueInputWriteResult = uniqueInputPtr->getDataStoreRef().copyFromBuffer(0, descending);
  SIMPLNX_RESULT_REQUIRE_VALID(uniqueInputWriteResult);
  Arguments uniqueArgs = filter.getDefaultArguments();
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
  uniqueArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  auto uniquePreflight = filter.preflight(uniqueData, uniqueArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(uniquePreflight.output_actions);
  auto uniqueResult = scope.executeFilter(filter, uniqueData, uniqueArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(uniqueResult.result);
  REQUIRE(uniqueData.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == k_UniqueCount);
  REQUIRE(uniqueData.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == 128.0F);
  REQUIRE(uniqueData.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == k_UniqueCount);
  REQUIRE(uniqueData.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == expectedModes);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: sparse negative range and empty selection semantics", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  SECTION("ShrinkToFit preserves negative and sparse FeatureIds")
  {
    DataStructure dataStructure;
    auto* groupPtr = DataGroup::Create(dataStructure, "Input");
    REQUIRE(groupPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {5}, {1}, groupPtr->getId());
    auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {5}, {1}, groupPtr->getId());
    auto* maskPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {5}, {1}, groupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(featureIdsPtr != nullptr);
    REQUIRE(maskPtr != nullptr);
    const std::array<int32, 5> input = {10, 20, 30, 40, 50};
    const std::array<int32, 5> ids = {-2, 0, 3, 3, -2};
    const std::array<uint8, 5> maskValues = {1, 1, 1, 0, 1};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, ids);
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
    auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, maskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);

    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), DataPath({"Input", "FeatureIds"}), DataPath({"Input", "Mask"}),
                                                  to_underlying(ComputeArrayStatistics::FeatureIdRangeControls::ShrinkToFit));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::array<int32, 6> expectedMapping = {-2, -1, 0, 1, 2, 3};
    const std::array<uint64, 6> expectedLength = {2, 0, 1, 0, 0, 1};
    const std::array<bool, 6> expectedHasData = {true, false, true, false, false, true};
    std::array<int32, 6> actualMapping = {};
    std::array<uint64, 6> actualLength = {};
    std::array<bool, 6> actualHasData = {};
    auto reducedFeatureIdsIndicesReadResult = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Reduced Feature Ids Indices"})).getDataStoreRef().copyIntoBuffer(0, actualMapping);
    SIMPLNX_RESULT_REQUIRE_VALID(reducedFeatureIdsIndicesReadResult);
    auto lengthReadResult = dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"})).getDataStoreRef().copyIntoBuffer(0, actualLength);
    SIMPLNX_RESULT_REQUIRE_VALID(lengthReadResult);
    auto featureHasDataReadResult = dataStructure.getDataRefAs<BoolArrayType>(DataPath({"Statistics", "FeatureHasData"})).getDataStoreRef().copyIntoBuffer(0, actualHasData);
    SIMPLNX_RESULT_REQUIRE_VALID(featureHasDataReadResult);
    REQUIRE(actualMapping == expectedMapping);
    REQUIRE(actualLength == expectedLength);
    REQUIRE(actualHasData == expectedHasData);
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == 30.0F);
    REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 2);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{10, 50});
    const auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Input", "Standardized"}));
    REQUIRE(standardized[0] == Approx(-1.0F));
    REQUIRE(std::isnan(standardized[1]));
    REQUIRE(std::isnan(standardized[2]));
    REQUIRE(standardized[3] == 0.0F);
    REQUIRE(standardized[4] == Approx(1.0F));
  }

  SECTION("all-negative None range is a valid zero-group result")
  {
    DataStructure dataStructure;
    auto* groupPtr = DataGroup::Create(dataStructure, "Input");
    REQUIRE(groupPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {3}, {1}, groupPtr->getId());
    auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {3}, {1}, groupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(featureIdsPtr != nullptr);
    const std::array<int32, 3> input = {10, 20, 30};
    const std::array<int32, 3> ids = {-3, -2, -1};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, ids);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Statistics"})).getNumberOfTuples() == 0);
    REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"})).getNumberOfTuples() == 0);
  }

  SECTION("all-false UInt8 mask produces initialized global outputs")
  {
    DataStructure dataStructure;
    auto* groupPtr = DataGroup::Create(dataStructure, "Input");
    REQUIRE(groupPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, groupPtr->getId());
    auto* maskPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {4}, {1}, groupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(maskPtr != nullptr);
    const std::array<int32, 4> input = {1, 2, 3, 4};
    const std::array<uint8, 4> maskValues = {0, 0, 0, 0};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, maskValues);
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}), {}, DataPath({"Input", "Mask"}));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Minimum"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Maximum"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Median"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"}))[0] == 0.0F);
    REQUIRE(dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "NumUniqueValues"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0).empty());
    std::array<float32, 4> standardized = {};
    auto standardizedReadResult = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, standardized);
    SIMPLNX_RESULT_REQUIRE_VALID(standardizedReadResult);
    REQUIRE(standardized == std::array<float32, 4>{0, 0, 0, 0});
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Scanline range and degenerate edges", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }

  SECTION("zero tuples")
  {
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    REQUIRE(inputGroupPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {0}, {1}, inputGroupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0).empty());
    REQUIRE(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Input", "Standardized"})).getNumberOfTuples() == 0);
  }

  SECTION("Custom and padded maximum minus one use the observed maximum")
  {
    const auto runRange = [&scanlineScenario](ComputeArrayStatistics::FeatureIdRangeControls rangeType, std::vector<int32> range) {
      DataStructure dataStructure;
      auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
      REQUIRE(inputGroupPtr != nullptr);
      auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroupPtr->getId());
      auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {4}, {1}, inputGroupPtr->getId());
      REQUIRE(valuesPtr != nullptr);
      REQUIRE(featureIdsPtr != nullptr);
      const std::array<int32, 4> input = {2, 5, 5, 9};
      const std::array<int32, 4> ids = {2, 5, 5, 9};
      auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
      SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
      auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, ids);
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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
      unit_test::AlgorithmTestScope scope(*scanlineScenario);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      const auto& mapping = dataStructure.getDataRefAs<Int32ArrayType>(DataPath({"Statistics", "Reduced Feature Ids Indices"}));
      const auto& length = dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({"Statistics", "Length"}));
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
      auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
      REQUIRE(inputGroupPtr != nullptr);
      auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {2}, {1}, inputGroupPtr->getId());
      auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {2}, {1}, inputGroupPtr->getId());
      REQUIRE(valuesPtr != nullptr);
      REQUIRE(featureIdsPtr != nullptr);
      const std::array<int32, 2> input = {1, 2};
      auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
      SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
      auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, ids);
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
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
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
    auto* inputGroupPtr = DataGroup::Create(constantData, "Input");
    REQUIRE(inputGroupPtr != nullptr);
    auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(constantData, "Values", {3}, {1}, inputGroupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    const std::array<int32, 3> input = {7, 7, 7};
    auto valuesWriteResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
    SIMPLNX_RESULT_REQUIRE_VALID(valuesWriteResult);
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    auto preflightResult = filter.preflight(constantData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    unit_test::AlgorithmTestScope constantScope(*scanlineScenario);
    auto executeResult = constantScope.executeFilter(filter, constantData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(constantData.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[0] == 0.0F);
    REQUIRE(constantData.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0) == std::vector<int32>{7});
    const auto& standardized = constantData.getDataRefAs<Float32ArrayType>(DataPath({"Input", "Standardized"}));
    REQUIRE(std::isnan(standardized[0]));
    REQUIRE(std::isnan(standardized[1]));
    REQUIRE(std::isnan(standardized[2]));

    DataStructure extremeData;
    auto* extremeInputGroupPtr = DataGroup::Create(extremeData, "Input");
    REQUIRE(extremeInputGroupPtr != nullptr);
    auto* extremesPtr = Int64ArrayType::CreateWithStore<DataStore<int64>>(extremeData, "Values", {2}, {1}, extremeInputGroupPtr->getId());
    REQUIRE(extremesPtr != nullptr);
    const std::array<int64, 2> extremeInput = {std::numeric_limits<int64>::lowest(), std::numeric_limits<int64>::max()};
    auto extremesWriteResult = extremesPtr->getDataStoreRef().copyFromBuffer(0, extremeInput);
    SIMPLNX_RESULT_REQUIRE_VALID(extremesWriteResult);
    Arguments extremeArgs = filter.getDefaultArguments();
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
    auto extremePreflight = filter.preflight(extremeData, extremeArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(extremePreflight.output_actions);
    unit_test::AlgorithmTestScope extremeScope(*scanlineScenario);
    auto extremeResult = extremeScope.executeFilter(filter, extremeData, extremeArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(extremeResult.result);
    REQUIRE(extremeData.getDataRefAs<Int64ArrayType>(DataPath({"Statistics", "Minimum"}))[0] == std::numeric_limits<int64>::lowest());
    REQUIRE(extremeData.getDataRefAs<Int64ArrayType>(DataPath({"Statistics", "Maximum"}))[0] == std::numeric_limits<int64>::max());
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: every supported scalar type", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
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

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {11}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {11}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 11);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);

    auto lengthVal = (*lengthArrayPtr)[0];
    auto minVal = (*minArrayPtr)[0];
    auto maxVal = (*maxArrayPtr)[0];
    auto meanVal = (*meanArrayPtr)[0];
    auto medianVal = (*medianArrayPtr)[0];
    auto modeVals = (*modeArrayPtr).getList(0);
    auto stdVal = (*stdArrayPtr)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArrayPtr)[0];
    auto numUnique = (*numUniqueValuesArrayPtr)[0];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 45);
    REQUIRE(modeVals.size() == 1);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(meanVal - 14.3333f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal - 10.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stdVal - 13.02f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal - 129.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique == 8);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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
    REQUIRE(std::fabs(stand0 - -1.01999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand1 - 0.43999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand2 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand3 - 2.35999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand4 - -0.70999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand5 - 0.12999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand6 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand7 - .58999f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand8 - -.4f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -.33f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
  Int32ArrayType* testFeatIdsArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroupPtr->getId());
  auto& testFeatIdsDataStore = testFeatIdsArrayPtr->getDataStoreRef();
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    REQUIRE(lengthArrayPtr->getNumberOfTuples() == 3);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    REQUIRE(minArrayPtr->getNumberOfTuples() == 3);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    REQUIRE(maxArrayPtr->getNumberOfTuples() == 3);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    REQUIRE(meanArrayPtr->getNumberOfTuples() == 3);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    REQUIRE(medianArrayPtr->getNumberOfTuples() == 3);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    REQUIRE(modeArrayPtr->getNumberOfLists() == 3);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    REQUIRE(stdArrayPtr->getNumberOfTuples() == 3);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    REQUIRE(sumArrayPtr->getNumberOfTuples() == 3);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 12);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);
    REQUIRE(numUniqueValuesArrayPtr->getNumberOfTuples() == 3);

    auto lengthVal1 = (*lengthArrayPtr)[0];
    auto lengthVal2 = (*lengthArrayPtr)[1];
    auto lengthVal3 = (*lengthArrayPtr)[2];
    auto minVal1 = (*minArrayPtr)[0];
    auto minVal2 = (*minArrayPtr)[1];
    auto minVal3 = (*minArrayPtr)[2];
    auto maxVal1 = (*maxArrayPtr)[0];
    auto maxVal2 = (*maxArrayPtr)[1];
    auto maxVal3 = (*maxArrayPtr)[2];
    auto meanVal1 = (*meanArrayPtr)[0];
    auto meanVal2 = (*meanArrayPtr)[1];
    auto meanVal3 = (*meanArrayPtr)[2];
    auto medianVal1 = (*medianArrayPtr)[0];
    auto medianVal2 = (*medianArrayPtr)[1];
    auto medianVal3 = (*medianArrayPtr)[2];
    auto modes0 = (*modeArrayPtr).getList(0);
    auto modes1 = (*modeArrayPtr).getList(1);
    auto modes2 = (*modeArrayPtr).getList(2);
    auto stdVal1 = (*stdArrayPtr)[0];
    auto stdVal2 = (*stdArrayPtr)[1];
    auto stdVal3 = (*stdArrayPtr)[2];
    auto sumVal1 = (*sumArrayPtr)[0];
    auto sumVal2 = (*sumArrayPtr)[1];
    auto sumVal3 = (*sumArrayPtr)[2];
    auto numUnique1 = (*numUniqueValuesArrayPtr)[0];
    auto numUnique2 = (*numUniqueValuesArrayPtr)[1];
    auto numUnique3 = (*numUniqueValuesArrayPtr)[2];

    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < unit_test::k_Epsilon);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique1 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < unit_test::k_Epsilon);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique2 == 3);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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

    REQUIRE(std::fabs(stand0 - -1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand1 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand4 - -1.669649f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand5 - 0.122169f) < unit_test::k_Epsilon);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand8 - 1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -1.0f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Ignore 0", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
  Int32ArrayType* testFeatIdsArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroupPtr->getId());
  auto& testFeatIdsDataStore = testFeatIdsArrayPtr->getDataStoreRef();
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    REQUIRE(lengthArrayPtr->getNumberOfTuples() == 2);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    REQUIRE(minArrayPtr->getNumberOfTuples() == 2);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    REQUIRE(maxArrayPtr->getNumberOfTuples() == 2);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    REQUIRE(meanArrayPtr->getNumberOfTuples() == 2);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    REQUIRE(medianArrayPtr->getNumberOfTuples() == 2);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    REQUIRE(modeArrayPtr->getNumberOfLists() == 2);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    REQUIRE(stdArrayPtr->getNumberOfTuples() == 2);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    REQUIRE(sumArrayPtr->getNumberOfTuples() == 2);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 12);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);
    REQUIRE(numUniqueValuesArrayPtr->getNumberOfTuples() == 2);
    auto* featureIdMappingArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArrayPtr != nullptr);
    REQUIRE(featureIdMappingArrayPtr->getNumberOfTuples() == 2);

    auto lengthVal1 = (*lengthArrayPtr)[0];
    auto lengthVal2 = (*lengthArrayPtr)[1];
    auto minVal1 = (*minArrayPtr)[0];
    auto minVal2 = (*minArrayPtr)[1];
    auto maxVal1 = (*maxArrayPtr)[0];
    auto maxVal2 = (*maxArrayPtr)[1];
    auto meanVal1 = (*meanArrayPtr)[0];
    auto meanVal2 = (*meanArrayPtr)[1];
    auto medianVal1 = (*medianArrayPtr)[0];
    auto medianVal2 = (*medianArrayPtr)[1];
    auto modes1 = (*modeArrayPtr).getList(0);
    auto modes2 = (*modeArrayPtr).getList(1);
    auto stdVal1 = (*stdArrayPtr)[0];
    auto stdVal2 = (*stdArrayPtr)[1];
    auto sumVal1 = (*sumArrayPtr)[0];
    auto sumVal2 = (*sumArrayPtr)[1];
    auto numUnique1 = (*numUniqueValuesArrayPtr)[0];
    auto numUnique2 = (*numUniqueValuesArrayPtr)[1];
    auto mapping1 = (*featureIdMappingArrayPtr)[0];
    auto mapping2 = (*featureIdMappingArrayPtr)[1];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < unit_test::k_Epsilon);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique2 == 2);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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

    REQUIRE(std::fabs(stand0 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand1 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand4 - -1.669649f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand5 - 0.122169f) < unit_test::k_Epsilon);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand8 - 1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -1.0f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Shrink to Fit", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
  Int32ArrayType* testFeatIdsArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroupPtr->getId());
  auto& testFeatIdsDataStore = testFeatIdsArrayPtr->getDataStoreRef();
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    REQUIRE(lengthArrayPtr->getNumberOfTuples() == 3);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    REQUIRE(minArrayPtr->getNumberOfTuples() == 3);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    REQUIRE(maxArrayPtr->getNumberOfTuples() == 3);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    REQUIRE(meanArrayPtr->getNumberOfTuples() == 3);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    REQUIRE(medianArrayPtr->getNumberOfTuples() == 3);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    REQUIRE(modeArrayPtr->getNumberOfLists() == 3);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    REQUIRE(stdArrayPtr->getNumberOfTuples() == 3);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    REQUIRE(sumArrayPtr->getNumberOfTuples() == 3);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 12);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);
    REQUIRE(numUniqueValuesArrayPtr->getNumberOfTuples() == 3);
    auto* featureIdMappingArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArrayPtr != nullptr);
    REQUIRE(featureIdMappingArrayPtr->getNumberOfTuples() == 3);

    auto lengthVal1 = (*lengthArrayPtr)[0];
    auto lengthVal2 = (*lengthArrayPtr)[1];
    auto lengthVal3 = (*lengthArrayPtr)[2];
    auto minVal1 = (*minArrayPtr)[0];
    auto minVal2 = (*minArrayPtr)[1];
    auto minVal3 = (*minArrayPtr)[2];
    auto maxVal1 = (*maxArrayPtr)[0];
    auto maxVal2 = (*maxArrayPtr)[1];
    auto maxVal3 = (*maxArrayPtr)[2];
    auto meanVal1 = (*meanArrayPtr)[0];
    auto meanVal2 = (*meanArrayPtr)[1];
    auto meanVal3 = (*meanArrayPtr)[2];
    auto medianVal1 = (*medianArrayPtr)[0];
    auto medianVal2 = (*medianArrayPtr)[1];
    auto medianVal3 = (*medianArrayPtr)[2];
    auto modes0 = (*modeArrayPtr).getList(0);
    auto modes1 = (*modeArrayPtr).getList(1);
    auto modes2 = (*modeArrayPtr).getList(2);
    auto stdVal1 = (*stdArrayPtr)[0];
    auto stdVal2 = (*stdArrayPtr)[1];
    auto stdVal3 = (*stdArrayPtr)[2];
    auto sumVal1 = (*sumArrayPtr)[0];
    auto sumVal2 = (*sumArrayPtr)[1];
    auto sumVal3 = (*sumArrayPtr)[2];
    auto numUnique1 = (*numUniqueValuesArrayPtr)[0];
    auto numUnique2 = (*numUniqueValuesArrayPtr)[1];
    auto numUnique3 = (*numUniqueValuesArrayPtr)[2];
    auto mapping1 = (*featureIdMappingArrayPtr)[0];
    auto mapping2 = (*featureIdMappingArrayPtr)[1];
    auto mapping3 = (*featureIdMappingArrayPtr)[2];

    REQUIRE(mapping1 == 0);
    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < unit_test::k_Epsilon);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique1 == 2);
    REQUIRE(mapping2 == 1);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < unit_test::k_Epsilon);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique2 == 3);
    REQUIRE(mapping3 == 2);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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

    REQUIRE(std::fabs(stand0 - -1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand1 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand4 - -1.669649f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand5 - 0.122169f) < unit_test::k_Epsilon);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand8 - 1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -1.0f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Padded Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
  Int32ArrayType* testFeatIdsArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroupPtr->getId());
  auto& testFeatIdsDataStore = testFeatIdsArrayPtr->getDataStoreRef();
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
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32ParameterType::ValueType>({1, 4}));

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    REQUIRE(lengthArrayPtr->getNumberOfTuples() == 4);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    REQUIRE(minArrayPtr->getNumberOfTuples() == 4);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    REQUIRE(maxArrayPtr->getNumberOfTuples() == 4);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    REQUIRE(meanArrayPtr->getNumberOfTuples() == 4);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    REQUIRE(medianArrayPtr->getNumberOfTuples() == 4);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    REQUIRE(modeArrayPtr->getNumberOfLists() == 4);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    REQUIRE(stdArrayPtr->getNumberOfTuples() == 4);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    REQUIRE(sumArrayPtr->getNumberOfTuples() == 4);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 12);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);
    REQUIRE(numUniqueValuesArrayPtr->getNumberOfTuples() == 4);
    auto* featureIdMappingArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArrayPtr != nullptr);
    REQUIRE(featureIdMappingArrayPtr->getNumberOfTuples() == 4);
    auto* featureHasDataArrayPtr = dataStructure.getDataAs<BoolArrayType>(statsDataPath.createChildPath(featureHasData));
    REQUIRE(featureHasDataArrayPtr != nullptr);
    REQUIRE(featureHasDataArrayPtr->getNumberOfTuples() == 4);

    auto lengthVal1 = (*lengthArrayPtr)[0];
    auto lengthVal2 = (*lengthArrayPtr)[1];
    auto minVal1 = (*minArrayPtr)[0];
    auto minVal2 = (*minArrayPtr)[1];
    auto maxVal1 = (*maxArrayPtr)[0];
    auto maxVal2 = (*maxArrayPtr)[1];
    auto meanVal1 = (*meanArrayPtr)[0];
    auto meanVal2 = (*meanArrayPtr)[1];
    auto medianVal1 = (*medianArrayPtr)[0];
    auto medianVal2 = (*medianArrayPtr)[1];
    auto modes1 = (*modeArrayPtr).getList(0);
    auto modes2 = (*modeArrayPtr).getList(1);
    auto stdVal1 = (*stdArrayPtr)[0];
    auto stdVal2 = (*stdArrayPtr)[1];
    auto sumVal1 = (*sumArrayPtr)[0];
    auto sumVal2 = (*sumArrayPtr)[1];
    auto numUnique1 = (*numUniqueValuesArrayPtr)[0];
    auto numUnique2 = (*numUniqueValuesArrayPtr)[1];
    auto mapping1 = (*featureIdMappingArrayPtr)[0];
    auto mapping2 = (*featureIdMappingArrayPtr)[1];
    auto mapping3 = (*featureIdMappingArrayPtr)[2];
    auto mapping4 = (*featureIdMappingArrayPtr)[3];
    auto featureHasData1 = (*featureHasDataArrayPtr)[0];
    auto featureHasData2 = (*featureHasDataArrayPtr)[1];
    auto featureHasData3 = (*featureHasDataArrayPtr)[2];
    auto featureHasData4 = (*featureHasDataArrayPtr)[3];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < unit_test::k_Epsilon);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique2 == 2);
    REQUIRE(mapping3 == 3);
    REQUIRE(mapping4 == 4);
    REQUIRE(featureHasData1);
    REQUIRE(featureHasData2);
    REQUIRE(!featureHasData3);
    REQUIRE(!featureHasData4);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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

    REQUIRE(std::fabs(stand0 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand1 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand4 - -1.669649f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand5 - 0.122169f) < unit_test::k_Epsilon);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand8 - 1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -1.0f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroupPtr->getId());
  auto& testInputDataStore = testInputArrayPtr->getDataStoreRef();
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
  BoolArrayType* maskArrayPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroupPtr->getId());
  auto& maskDataStore = maskArrayPtr->getDataStoreRef();
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
  Int32ArrayType* testFeatIdsArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroupPtr->getId());
  auto& testFeatIdsDataStore = testFeatIdsArrayPtr->getDataStoreRef();
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
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32ParameterType::ValueType>({2, 4}));

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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArrayPtr != nullptr);
    REQUIRE(lengthArrayPtr->getNumberOfTuples() == 1);
    auto* minArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(min));
    REQUIRE(minArrayPtr != nullptr);
    REQUIRE(minArrayPtr->getNumberOfTuples() == 1);
    auto* maxArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(max));
    REQUIRE(maxArrayPtr != nullptr);
    REQUIRE(maxArrayPtr->getNumberOfTuples() == 1);
    auto* meanArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArrayPtr != nullptr);
    REQUIRE(meanArrayPtr->getNumberOfTuples() == 1);
    auto* medianArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(median));
    REQUIRE(medianArrayPtr != nullptr);
    REQUIRE(medianArrayPtr->getNumberOfTuples() == 1);
    auto* modeArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArrayPtr != nullptr);
    REQUIRE(modeArrayPtr->getNumberOfLists() == 1);
    auto* stdArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(std));
    REQUIRE(stdArrayPtr != nullptr);
    REQUIRE(stdArrayPtr->getNumberOfTuples() == 1);
    auto* sumArrayPtr = dataStructure.getDataAs<Float32ArrayType>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArrayPtr != nullptr);
    REQUIRE(sumArrayPtr->getNumberOfTuples() == 1);
    auto* standardizeArrayPtr = dataStructure.getDataAs<Float32ArrayType>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArrayPtr != nullptr);
    REQUIRE(standardizeArrayPtr->getNumberOfTuples() == 12);
    auto* numUniqueValuesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArrayPtr != nullptr);
    REQUIRE(numUniqueValuesArrayPtr->getNumberOfTuples() == 1);
    auto* featureIdMappingArrayPtr = dataStructure.getDataAs<Int32ArrayType>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArrayPtr != nullptr);
    REQUIRE(featureIdMappingArrayPtr->getNumberOfTuples() == 1);

    auto lengthVal1 = (*lengthArrayPtr)[0];
    auto minVal1 = (*minArrayPtr)[0];
    auto maxVal1 = (*maxArrayPtr)[0];
    auto meanVal1 = (*meanArrayPtr)[0];
    auto medianVal1 = (*medianArrayPtr)[0];
    auto modes1 = (*modeArrayPtr).getList(0);
    auto stdVal1 = (*stdArrayPtr)[0];
    auto sumVal1 = (*sumArrayPtr)[0];
    auto numUnique1 = (*numUniqueValuesArrayPtr)[0];
    auto mapping1 = (*featureIdMappingArrayPtr)[0];

    REQUIRE(mapping1 == 2);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 10);
    REQUIRE(maxVal1 == 22);
    REQUIRE(std::fabs(meanVal1 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(medianVal1 - 16.0f) < unit_test::k_Epsilon);
    REQUIRE(modes1.size() == 2);
    REQUIRE(VectorContains(modes1, 10) == true);
    REQUIRE(VectorContains(modes1, 22) == true);
    REQUIRE(std::fabs(stdVal1 - 6.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(sumVal1 - 64.0f) < unit_test::k_Epsilon);
    REQUIRE(numUnique1 == 2);

    const auto& standardizeDataStore = standardizeArrayPtr->getDataStoreRef();
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
    REQUIRE(std::fabs(stand8 - 1.0f) < unit_test::k_Epsilon);
    REQUIRE(std::fabs(stand9 - -1.0f) < unit_test::k_Epsilon);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeArrayStatisticsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeArrayStatisticsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeArrayStatisticsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeArrayStatisticsFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Boolean group statistics use the selected population", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const bool useMask = GENERATE(false, true);
  const bool findMean = GENERATE(false, true);
  CAPTURE(scenario, useMask, findMean);
  DataStructure dataStructure;
  const usize count = useMask ? 5 : 3;
  auto* valuesPtr = BoolArrayType::Create(dataStructure, "Values", data_store_utilities::CreateDataStore<bool>(dataStructure, DataPath({"Values"}), {count}, {1}));
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"Ids"}), {count}, {1}));
  auto* maskPtr = BoolArrayType::Create(dataStructure, "Mask", data_store_utilities::CreateDataStore<bool>(dataStructure, DataPath({"Mask"}), {count}, {1}));
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  const std::array<bool, 5> input = {true, true, false, true, false};
  for(usize i = 0; i < count; ++i)
  {
    (*valuesPtr)[i] = input[i];
    (*idsPtr)[i] = 1;
    (*maskPtr)[i] = i < 3;
  }
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Ids"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(findMean));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(findMean));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(findMean)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})));
    dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})).fill(12345.0F);
    auto inputValues = CreateMinimalStatisticsInputValues(DataPath({"Values"}), DataPath({"Statistics"}));
    inputValues.compute_by_index = true;
    inputValues.feature_ids_array_path = DataPath({"Ids"});
    inputValues.use_mask = useMask;
    inputValues.mask_array_path = DataPath({"Mask"});
    inputValues.find_mean = true;
    inputValues.find_std_deviation = true;
    inputValues.standardize_data = true;
    const std::atomic_bool shouldCancel = false;
    auto seededResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(seededResult);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"})));
  // Two trues among three selected tuples give p=2/3 and sqrt(p*(1-p))=sqrt(2)/3.
  CHECK(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[1] == Approx(0.4714045208F));
  if(findMean)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"})));
    CHECK(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[1] == Approx(0.6666666667F));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})));
    const auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"}));
    // (1-p)/sqrt(p*(1-p))=1/sqrt(2), and -p/sqrt(p*(1-p))=-sqrt(2).
    CHECK(standardized[0] == Approx(0.7071067812F));
    CHECK(standardized[1] == Approx(0.7071067812F));
    CHECK(standardized[2] == Approx(-1.4142135624F));
    for(usize i = 3; i < count; ++i)
    {
      CHECK(standardized[i] == 12345.0F);
    }
  }
  for(usize i = 0; i < count; ++i)
  {
    CHECK((*valuesPtr)[i] == input[i]);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Constant Boolean groups retain unguarded standardization", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const bool useMask = GENERATE(false, true);
  CAPTURE(scenario, useMask);
  DataStructure dataStructure;
  auto* valuesPtr = BoolArrayType::Create(dataStructure, "Values", data_store_utilities::CreateDataStore<bool>(dataStructure, DataPath({"Values"}), {6}, {1}));
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"Ids"}), {6}, {1}));
  auto* maskPtr = BoolArrayType::Create(dataStructure, "Mask", data_store_utilities::CreateDataStore<bool>(dataStructure, DataPath({"Mask"}), {6}, {1}));
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  const std::array<bool, 6> input = {true, false, false, true, true, false};
  const std::array<int32, 6> featureIds = {1, 2, 2, 3, 3, 2};
  auto valueResult = valuesPtr->getDataStoreRef().copyFromBuffer(0, input);
  SIMPLNX_RESULT_REQUIRE_VALID(valueResult);
  auto idResult = idsPtr->getDataStoreRef().copyFromBuffer(0, featureIds);
  SIMPLNX_RESULT_REQUIRE_VALID(idResult);
  maskPtr->fill(true);
  (*maskPtr)[5] = false;
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Ids"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})));
    dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})).fill(12345.0F);
    auto inputValues = CreateMinimalStatisticsInputValues(DataPath({"Values"}), DataPath({"Statistics"}));
    inputValues.compute_by_index = true;
    inputValues.feature_ids_array_path = DataPath({"Ids"});
    inputValues.use_mask = useMask;
    inputValues.mask_array_path = DataPath({"Mask"});
    inputValues.find_mean = true;
    inputValues.find_std_deviation = true;
    inputValues.standardize_data = true;
    const std::atomic_bool shouldCancel = false;
    auto seededResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    SIMPLNX_RESULT_REQUIRE_VALID(seededResult);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"})));
  const auto& mean = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}));
  const auto& deviation = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}));
  const auto& standardized = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Standardized"}));
  // Constant groups have p=1 or p=0, zero deviation, and standardized values 0/0=NaN.
  CHECK(mean[1] == 1.0F);
  CHECK(mean[2] == 0.0F);
  CHECK(mean[3] == 1.0F);
  for(usize group = 1; group <= 3; ++group)
  {
    CHECK(deviation[group] == 0.0F);
  }
  for(usize i = 0; i < 6; ++i)
  {
    CHECK((*valuesPtr)[i] == input[i]);
    if(useMask && i == 5)
    {
      CHECK(standardized[i] == 12345.0F);
    }
    else
    {
      CHECK(std::isnan(standardized[i]));
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Boolean counts remain exact beyond float32 integers", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const bool grouped = GENERATE(false, true);
  CAPTURE(scenario, grouped);
  // 2^24+2 trues give the exactly representable sum 16777218, above float32's consecutive-integer range.
  constexpr usize k_TrueCount = 16777218;
  const usize count = k_TrueCount + (grouped ? 0 : 1);
  DataStructure dataStructure;
  auto* valuesPtr = BoolArrayType::Create(dataStructure, "Values", data_store_utilities::CreateDataStore<bool>(dataStructure, DataPath({"Values"}), {count}, {1}));
  REQUIRE(valuesPtr != nullptr);
  valuesPtr->fill(true);
  ComputeArrayStatisticsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Values"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(grouped));
  if(grouped)
  {
    auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", data_store_utilities::CreateDataStore<int32>(dataStructure, DataPath({"Ids"}), {count}, {1}));
    REQUIRE(idsPtr != nullptr);
    idsPtr->fill(1);
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Ids"})));
  }
  else
  {
    (*valuesPtr)[count - 1] = false;
  }
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"})));
  const usize group = grouped ? 1 : 0;
  CHECK(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Summation"}))[group] == 16777218.0F);
  // The ungrouped fraction 16777218/16777219 rounds to the float32 immediately below 1.
  CHECK(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "Mean"}))[group] == (grouped ? 1.0F : 0.9999999403953552F));
  if(grouped)
  {
    CHECK(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"Statistics", "StandardDeviation"}))[group] == 0.0F);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

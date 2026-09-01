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

#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <nonstd/span.hpp>
#include <optional>
#include <vector>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
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
  SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
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
  SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
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

#if SIMPLNX_TEST_ALGORITHM_PATH != 1
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
  SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
  SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, ids));
  std::optional<DataPath> maskPath;
  if(maskKind == 1)
  {
    auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
    REQUIRE(mask != nullptr);
    SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, boolMaskValues));
    maskPath = DataPath({"Input", "Mask"});
  }
  else if(maskKind == 2)
  {
    auto* mask = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {k_TupleCount}, {1}, inputGroup->getId());
    REQUIRE(mask != nullptr);
    SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, uint8MaskValues));
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
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<UInt64Array>(statisticsPath.createChildPath("Length")).getDataStoreRef().copyIntoBuffer(0, result.Length));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("Minimum")).getDataStoreRef().copyIntoBuffer(0, result.Minimum));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("Maximum")).getDataStoreRef().copyIntoBuffer(0, result.Maximum));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Mean")).getDataStoreRef().copyIntoBuffer(0, result.Mean));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Median")).getDataStoreRef().copyIntoBuffer(0, result.Median));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("StandardDeviation")).getDataStoreRef().copyIntoBuffer(0, result.StandardDeviation));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(statisticsPath.createChildPath("Summation")).getDataStoreRef().copyIntoBuffer(0, result.Summation));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Int32Array>(statisticsPath.createChildPath("NumUniqueValues")).getDataStoreRef().copyIntoBuffer(0, result.Unique));
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, result.Standardized));
  if(computeByIndex)
  {
    result.FeatureHasData.resize(groupCount);
    auto featureHasData = std::make_unique<bool[]>(groupCount);
    SIMPLNX_RESULT_REQUIRE_VALID(
        dataStructure.getDataRefAs<BoolArray>(statisticsPath.createChildPath("FeatureHasData")).getDataStoreRef().copyIntoBuffer(0, nonstd::span<bool>(featureHasData.get(), groupCount)));
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
#endif

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

#if SIMPLNX_TEST_ALGORITHM_PATH != 1
TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Direct and Scanline mask grouping parity", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  const bool computeByIndex = GENERATE(false, true);
  const int32 maskKind = GENERATE(0, 1, 2);
  CAPTURE(computeByIndex, maskKind);
  StatisticsParityResult direct;
  {
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
    direct = RunStatisticsParityCase(scope, computeByIndex, maskKind);
  }
  StatisticsParityResult scanline;
  {
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    scanline = RunStatisticsParityCase(scope, computeByIndex, maskKind);
  }
  REQUIRE(scanline.Length == direct.Length);
  REQUIRE(scanline.Minimum == direct.Minimum);
  REQUIRE(scanline.Maximum == direct.Maximum);
  REQUIRE(scanline.Unique == direct.Unique);
  REQUIRE(scanline.FeatureHasData == direct.FeatureHasData);
  REQUIRE(scanline.Modes == direct.Modes);
  const auto requireFloatParity = [](const std::vector<float32>& actual, const std::vector<float32>& expected) {
    REQUIRE(actual.size() == expected.size());
    for(usize index = 0; index < actual.size(); ++index)
    {
      if(std::isnan(expected[index]))
      {
        REQUIRE(std::isnan(actual[index]));
      }
      else
      {
        REQUIRE(actual[index] == Approx(expected[index]).margin(1.0E-5F));
      }
    }
  };
  requireFloatParity(scanline.Mean, direct.Mean);
  requireFloatParity(scanline.Median, direct.Median);
  requireFloatParity(scanline.StandardDeviation, direct.StandardDeviation);
  requireFloatParity(scanline.Summation, direct.Summation);
  requireFloatParity(scanline.Standardized, direct.Standardized);
}
#endif

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Scanline propagates bulk store failures", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
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
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));

    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    const std::atomic_bool shouldCancel = false;
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    const auto executeResult = scope.execute([&] { return ComputeArrayStatistics(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: entry and mid-pass cancellation preserve unwritten outputs", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

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
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
    minimum->fill(93);
    ComputeArrayStatisticsInputValues inputValues = CreateMinimalStatisticsInputValues(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    inputValues.FindLength = true;
    inputValues.FindMin = true;
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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
  SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(input.data(), input.size())));
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

#if SIMPLNX_TEST_ALGORITHM_PATH == 2
TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: bounded exact fallback without external sort", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();
  REQUIRE_FALSE(DataStoreUtilities::GetIOCollection().hasExternalSortCapability());
  UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "Input");
  REQUIRE(group != nullptr);
  auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {6}, {1}, group->getId());
  auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {6}, {1}, group->getId());
  REQUIRE(values != nullptr);
  REQUIRE(mask != nullptr);
  const std::array<int32, 6> input = {5, 1, 5, 3, 3, 9};
  const std::array<bool, 6> maskValues = {true, true, true, true, true, true};
  SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
  SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, maskValues));

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
  SIMPLNX_RESULT_REQUIRE_VALID(uniqueInput->getDataStoreRef().copyFromBuffer(0, descending));
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
#endif

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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, ids));
    SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, maskValues));

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
    SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Reduced Feature Ids Indices"})).getDataStoreRef().copyIntoBuffer(0, actualMapping));
    SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"})).getDataStoreRef().copyIntoBuffer(0, actualLength));
    SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<BoolArray>(DataPath({"Statistics", "FeatureHasData"})).getDataStoreRef().copyIntoBuffer(0, actualHasData));
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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
    SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, ids));
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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
    SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, maskValues));
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
    SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getDataStoreRef().copyIntoBuffer(0, standardized));
    REQUIRE(standardized == std::array<float32, 4>{0, 0, 0, 0});
  }
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Scanline range and degenerate edges", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

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
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}))[0] == 0);
    REQUIRE(dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Statistics", "Mode"})).getList(0).empty());
    REQUIRE(dataStructure.getDataRefAs<Float32Array>(DataPath({"Input", "Standardized"})).getNumberOfTuples() == 0);
  }

  SECTION("Custom and padded maximum minus one use the observed maximum")
  {
    const auto runRange = [](ComputeArrayStatistics::FeatureIdRangeControls rangeType, std::vector<int32> range) {
      DataStructure dataStructure;
      auto* inputGroup = DataGroup::Create(dataStructure, "Input");
      REQUIRE(inputGroup != nullptr);
      auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, inputGroup->getId());
      auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {4}, {1}, inputGroup->getId());
      REQUIRE(values != nullptr);
      REQUIRE(featureIds != nullptr);
      const std::array<int32, 4> input = {2, 5, 5, 9};
      const std::array<int32, 4> ids = {2, 5, 5, 9};
      SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
      SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, ids));
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
      UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
      auto executeResult = scope.executeFilter(filter, dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
      const auto& mapping = dataStructure.getDataRefAs<Int32Array>(DataPath({"Statistics", "Reduced Feature Ids Indices"}));
      const auto& length = dataStructure.getDataRefAs<UInt64Array>(DataPath({"Statistics", "Length"}));
      std::vector<int32> mappingValues(mapping.getNumberOfTuples());
      std::vector<uint64> lengthValues(length.getNumberOfTuples());
      SIMPLNX_RESULT_REQUIRE_VALID(mapping.getDataStoreRef().copyIntoBuffer(0, mappingValues));
      SIMPLNX_RESULT_REQUIRE_VALID(length.getDataStoreRef().copyIntoBuffer(0, lengthValues));
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
      SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
      SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, ids));
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
    SIMPLNX_RESULT_REQUIRE_VALID(values->getDataStoreRef().copyFromBuffer(0, input));
    ComputeArrayStatisticsFilter filter;
    Arguments args = CreateAllStatisticsArguments(DataPath({"Input", "Values"}), DataPath({"Statistics"}));
    auto preflightResult = filter.preflight(constantData, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    UnitTest::AlgorithmTestScope constantScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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
    SIMPLNX_RESULT_REQUIRE_VALID(extremes->getDataStoreRef().copyFromBuffer(0, extremeInput));
    Arguments extremeArgs = filter.getDefaultArguments();
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Values"})));
    extremeArgs.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Statistics"})));
    auto extremePreflight = filter.preflight(extremeData, extremeArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(extremePreflight.outputActions);
    UnitTest::AlgorithmTestScope extremeScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
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

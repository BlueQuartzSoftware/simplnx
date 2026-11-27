#include "SimplnxCore/Filters/Algorithms/ComputeArrayHistogramByFeature.hpp"
#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <limits>
#include <nonstd/span.hpp>
#include <numeric>
#include <optional>
#include <vector>

using namespace nx::core;

namespace
{
constexpr float64 k_MaxDifference = 0.0001;
constexpr StringLiteralType k_BinRangesName = "Ranges";
constexpr StringLiteralType k_BinCountsName = "Counts";
constexpr StringLiteralType k_Array0Name = "array0";
constexpr StringLiteralType k_Array1Name = "array1";
constexpr StringLiteralType k_Array2Name = "array2";

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
class FailingReadDataStore : public DataStore<T>
{
public:
  FailingReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize /*unused*/, nonstd::span<T> /*unused*/) const override
  {
    return MakeErrorResult(m_ErrorCode, "Injected histogram bulk-read failure");
  }

private:
  int32 m_ErrorCode = 0;
};

template <typename T>
class FailingWriteDataStore : public DataStore<T>
{
public:
  FailingWriteDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize /*unused*/, nonstd::span<const T> /*unused*/) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected histogram bulk-write failure");
  }

private:
  int32 m_ErrorCode = 0;
};

template <typename T>
class CancelAfterReadDataStore : public DataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel)
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

template <typename T, usize N>
void CompareHistograms(const AbstractDataStore<T>& calculated, const std::array<T, N>& actual)
{
  if(calculated.getSize() != actual.size())
  {
    throw std::runtime_error(fmt::format("Improper sizing of DataStore. {} vs {}", calculated.getSize(), actual.size()));
  }
  for(int32 i = 0; i < N; i++)
  {
    T diff = std::fabs(calculated[i] - actual[i]);
    REQUIRE(diff < ::k_MaxDifference);
  }
}

template <typename T>
void FillArray(DataArray<T>& data, const std::vector<T>& values)
{
  int32 count = 0;
  for(T value : values)
  {
    data.getDataStore()->setValue(count, value);
    count++;
  }
}

template <typename T>
void RunSparseTypeOracle(unit_test::AlgorithmTestScope& scope)
{
  const DataPath inputPath({"Input", "Values"});
  const DataPath featurePath({"Input", "FeatureIds"});
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(groupPtr != nullptr);
  auto* inputPtr = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, "Values", {8}, {1}, groupPtr->getId());
  auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {8}, {1}, groupPtr->getId());
  REQUIRE(inputPtr != nullptr);
  REQUIRE(featureIdsPtr != nullptr);
  const std::array<T, 8> values = {T{0}, T{1}, T{2}, T{3}, T{0}, T{1}, T{2}, T{3}};
  const std::array<int32, 8> features = {0, 0, 2, 2, 4, 4, -1, -1};
  auto inputWriteResult = inputPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputWriteResult);
  auto featureIdsWriteResult = featureIdsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(features.data(), features.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsWriteResult);
  scope.requireExpectedStore(*inputPtr);
  scope.requireExpectedStore(*featureIdsPtr);

  const auto execute = [&](bool customRange, const DataPath& groupPath) {
    ComputeArrayHistogramByFeatureFilter filter;
    Arguments args = filter.getDefaultArguments();
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(4));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UserDefinedRange_Key, std::make_any<bool>(customRange));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MinRange_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaxRange_Key, std::make_any<float64>(4.0));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputPath}));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featurePath));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(groupPath));
    auto result = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto& counts = dataStructure.getDataRefAs<UInt64ArrayType>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Bin Counts"));
    std::array<uint64, 20> actual = {};
    auto countsReadResult = counts.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(actual.data(), actual.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(countsReadResult);
    const auto& mostPopulated = dataStructure.getDataRefAs<UInt64ArrayType>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Most Populated Bin"));
    std::array<uint64, 10> mostPopulatedValues = {};
    auto mostPopulatedReadResult = mostPopulated.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostPopulatedValues.data(), mostPopulatedValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(mostPopulatedReadResult);
    REQUIRE(mostPopulatedValues[0] == 0);
    REQUIRE(mostPopulatedValues[1] == 1);
    REQUIRE(mostPopulatedValues[2] == 0);
    REQUIRE(mostPopulatedValues[3] == 0);
    const auto& ranges = dataStructure.getDataRefAs<DataArray<T>>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Bin Ranges"));
    std::array<T, 40> rangeValues = {};
    auto rangesReadResult = ranges.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(rangeValues.data(), rangeValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(rangesReadResult);
    for(usize index = 8; index < 16; ++index)
    {
      REQUIRE(rangeValues[index] == T{});
    }
    return actual;
  };

  const std::array<uint64, 20> fullExpected = {1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0};
  const std::array<uint64, 20> customExpected = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0};
  REQUIRE(execute(false, DataPath({"Full"})) == fullExpected);
  REQUIRE(execute(true, DataPath({"Custom"})) == customExpected);
}

std::vector<int32> ExecuteInt32ModalCase(unit_test::AlgorithmTestScope& scope, const std::vector<int32>& values, const std::vector<int32>& featureIds, int32 bins, float64 minimum, float64 maximum,
                                         bool customRange, const DataPath& outputPath, std::vector<uint64>* countsResult = nullptr, std::vector<int32>* rangesResult = nullptr,
                                         std::vector<std::vector<int32>>* modalResults = nullptr)
{
  const DataPath inputPath({"Input", "Values"});
  const DataPath featurePath({"Input", "FeatureIds"});
  DataStructure dataStructure;
  auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroupPtr != nullptr);
  auto* inputPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {values.size()}, {1}, inputGroupPtr->getId());
  auto* idsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {featureIds.size()}, {1}, inputGroupPtr->getId());
  REQUIRE(inputPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  auto inputWriteResult = inputPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputWriteResult);
  auto idsWriteResult = idsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(idsWriteResult);
  scope.requireExpectedStore(*inputPtr);
  scope.requireExpectedStore(*idsPtr);

  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(bins));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UserDefinedRange_Key, std::make_any<bool>(customRange));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MinRange_Key, std::make_any<float64>(minimum));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaxRange_Key, std::make_any<float64>(maximum));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputPath}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featurePath));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(outputPath));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto execute = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  const DataPath histogramPath = outputPath.createChildPath("\"Values\" Histogram");
  if(countsResult != nullptr)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64ArrayType>(histogramPath.createChildPath("Bin Counts")));
    const auto& counts = dataStructure.getDataRefAs<UInt64ArrayType>(histogramPath.createChildPath("Bin Counts"));
    countsResult->resize(counts.getDataStoreRef().getSize());
    auto countsReadResult = counts.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countsResult->data(), countsResult->size()));
    SIMPLNX_RESULT_REQUIRE_VALID(countsReadResult);
  }
  if(rangesResult != nullptr)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32ArrayType>(histogramPath.createChildPath("Bin Ranges")));
    const auto& ranges = dataStructure.getDataRefAs<Int32ArrayType>(histogramPath.createChildPath("Bin Ranges"));
    rangesResult->resize(ranges.getDataStoreRef().getSize());
    auto rangesReadResult = ranges.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangesResult->data(), rangesResult->size()));
    SIMPLNX_RESULT_REQUIRE_VALID(rangesReadResult);
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<int32>>(histogramPath.createChildPath("Modal Bin Ranges")));
  const auto& modal = dataStructure.getDataRefAs<NeighborList<int32>>(histogramPath.createChildPath("Modal Bin Ranges"));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
  if(modal.getNumberOfTuples() == 0)
  {
    return {};
  }
  if(modalResults != nullptr)
  {
    modalResults->resize(modal.getNumberOfTuples());
    for(usize feature = 0; feature < modal.getNumberOfTuples(); ++feature)
    {
      (*modalResults)[feature] = modal.getList(feature);
    }
  }
  return modal.getList(0);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: All Histogram Calculations", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroupPtr = DataGroup::Create(dataStructure, "TestData");
  DataPath histogramsDataPath({"Histograms"});
  std::string inputArrayName = "InputArray";
  DataPath inputArrayPath({"TestData", inputArrayName});
  DataPath featureIdsArrayPath({"TestData", "FeatureIds"});
  DataPath maskArrayPath({"TestData", "Mask"});
  Int32ArrayType* testInputArrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, inputArrayName, {12}, {1}, topLevelGroupPtr->getId());
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

  const std::string histogram = "Histogram";
  const std::string ranges = "Ranges";
  const std::string mostPopulatedBin = "Most Populated Bin";
  const std::string modalBinRanges = "Modal Bin Ranges";
  Arguments args;

  // Execute the configured filter.
  {
    ComputeArrayHistogramByFeatureFilter filter;

    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MinRange_Key, std::make_any<float64>(0));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaxRange_Key, std::make_any<float64>(100));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(5));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputArrayPath}));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsArrayPath));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskArrayPath));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(histogramsDataPath));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_HistoBinCountName_Key, std::make_any<std::string>(histogram));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(ranges));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_HistoMostPopulatedBinName_Key, std::make_any<std::string>(mostPopulatedBin));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_HistoModalBinRangesName_Key, std::make_any<std::string>(modalBinRanges));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath histogramPath = histogramsDataPath.createChildPath(fmt::format("\"{}\" Histogram", inputArrayName));
    auto* dataGroupPtr = dataStructure.getDataAs<DataGroup>(histogramPath);
    REQUIRE(dataGroupPtr != nullptr);
    auto* histArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(histogramPath.createChildPath(histogram));
    REQUIRE(histArrayPtr != nullptr);
    REQUIRE(histArrayPtr->getNumberOfTuples() == 3);
    REQUIRE(histArrayPtr->getNumberOfComponents() == 5);
    auto* rangesArrayPtr = dataStructure.getDataAs<Int32ArrayType>(histogramPath.createChildPath(ranges));
    REQUIRE(rangesArrayPtr != nullptr);
    REQUIRE(rangesArrayPtr->getNumberOfTuples() == 3);
    REQUIRE(rangesArrayPtr->getNumberOfComponents() == 10);
    auto* mostPopulatedBinArrayPtr = dataStructure.getDataAs<UInt64ArrayType>(histogramPath.createChildPath(mostPopulatedBin));
    REQUIRE(mostPopulatedBinArrayPtr != nullptr);
    REQUIRE(mostPopulatedBinArrayPtr->getNumberOfTuples() == 3);
    auto* modalBinRangesArrayPtr = dataStructure.getDataAs<NeighborList<int32>>(histogramPath.createChildPath(modalBinRanges));
    REQUIRE(modalBinRangesArrayPtr != nullptr);
    REQUIRE(modalBinRangesArrayPtr->getNumberOfTuples() == 3);

    auto modalBinRange0 = (*modalBinRangesArrayPtr).getList(0);
    auto modalBinRange1 = (*modalBinRangesArrayPtr).getList(1);
    auto modalBinRange2 = (*modalBinRangesArrayPtr).getList(2);

    REQUIRE((*histArrayPtr)[0] == 1);
    REQUIRE((*histArrayPtr)[1] == 0);
    REQUIRE((*histArrayPtr)[2] == 0);
    REQUIRE((*histArrayPtr)[3] == 0);
    REQUIRE((*histArrayPtr)[4] == 1);
    REQUIRE((*histArrayPtr)[5] == 1);
    REQUIRE((*histArrayPtr)[6] == 0);
    REQUIRE((*histArrayPtr)[7] == 0);
    REQUIRE((*histArrayPtr)[8] == 1);
    REQUIRE((*histArrayPtr)[9] == 2);
    REQUIRE((*histArrayPtr)[10] == 2);
    REQUIRE((*histArrayPtr)[11] == 0);
    REQUIRE((*histArrayPtr)[12] == 0);
    REQUIRE((*histArrayPtr)[13] == 0);
    REQUIRE((*histArrayPtr)[14] == 2);

    REQUIRE((*rangesArrayPtr)[0] == 1);
    REQUIRE((*rangesArrayPtr)[1] == 15);
    REQUIRE((*rangesArrayPtr)[2] == 15);
    REQUIRE((*rangesArrayPtr)[3] == 30);
    REQUIRE((*rangesArrayPtr)[4] == 30);
    REQUIRE((*rangesArrayPtr)[5] == 44);
    REQUIRE((*rangesArrayPtr)[6] == 44);
    REQUIRE((*rangesArrayPtr)[7] == 59);
    REQUIRE((*rangesArrayPtr)[8] == 59);
    REQUIRE((*rangesArrayPtr)[9] == 74);
    REQUIRE((*rangesArrayPtr)[10] == 5);
    REQUIRE((*rangesArrayPtr)[11] == 8);
    REQUIRE((*rangesArrayPtr)[12] == 8);
    REQUIRE((*rangesArrayPtr)[13] == 11);
    REQUIRE((*rangesArrayPtr)[14] == 11);
    REQUIRE((*rangesArrayPtr)[15] == 14);
    REQUIRE((*rangesArrayPtr)[16] == 14);
    REQUIRE((*rangesArrayPtr)[17] == 17);
    REQUIRE((*rangesArrayPtr)[18] == 17);
    REQUIRE((*rangesArrayPtr)[19] == 21);
    REQUIRE((*rangesArrayPtr)[20] == 10);
    REQUIRE((*rangesArrayPtr)[21] == 12);
    REQUIRE((*rangesArrayPtr)[22] == 12);
    REQUIRE((*rangesArrayPtr)[23] == 15);
    REQUIRE((*rangesArrayPtr)[24] == 15);
    REQUIRE((*rangesArrayPtr)[25] == 17);
    REQUIRE((*rangesArrayPtr)[26] == 17);
    REQUIRE((*rangesArrayPtr)[27] == 20);
    REQUIRE((*rangesArrayPtr)[28] == 20);
    REQUIRE((*rangesArrayPtr)[29] == 23);

    REQUIRE((*mostPopulatedBinArrayPtr)[0] == 0);
    REQUIRE((*mostPopulatedBinArrayPtr)[1] == 1);
    REQUIRE((*mostPopulatedBinArrayPtr)[2] == 4);
    REQUIRE((*mostPopulatedBinArrayPtr)[3] == 2);
    REQUIRE((*mostPopulatedBinArrayPtr)[4] == 0);
    REQUIRE((*mostPopulatedBinArrayPtr)[5] == 2);

    REQUIRE(modalBinRange0[0] == 1);
    REQUIRE(modalBinRange0[1] == 15);
    // Modes 1 and 73 occupy bins [1, 15] and [59, 74]; mode 20 occupies [17, 21].
    REQUIRE(modalBinRange0[2] == 59);
    REQUIRE(modalBinRange0[3] == 74);
    REQUIRE(modalBinRange1[0] == 17);
    REQUIRE(modalBinRange1[1] == 21);
    REQUIRE(modalBinRange2[0] == 10);
    REQUIRE(modalBinRange2[1] == 12);
    // Feature 2 has tied modes 10 and 22, in bins [10, 12] and [20, 23].
    REQUIRE(modalBinRange2[2] == 20);
    REQUIRE(modalBinRange2[3] == 23);
  }

  // No mask keeps the Scanline path on bulk reads without a mask buffer.
  {
    const DataPath noModalHistogramsPath({"HistogramsNoModal"});
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(noModalHistogramsPath));

    ComputeArrayHistogramByFeatureFilter filter;
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath histogramPath = noModalHistogramsPath.createChildPath(fmt::format("\"{}\" Histogram", inputArrayName)).createChildPath(histogram);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64ArrayType>(histogramPath));
    const auto& histogramArray = dataStructure.getDataRefAs<UInt64ArrayType>(histogramPath);
    REQUIRE(histogramArray.getDataStoreRef()[0] == 1);
    REQUIRE(histogramArray.getDataStoreRef()[4] == 1);
    REQUIRE(histogramArray.getDataStoreRef()[9] == 2);
    REQUIRE(histogramArray.getDataStoreRef()[10] == 2);
    REQUIRE(histogramArray.getDataStoreRef()[14] == 3);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Reports out-of-range counts per input array", "[SimplnxCore][ComputeArrayHistogramByFeature]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const auto createArray = [&](const std::string& name, const std::vector<int32>& values) {
    auto* arrayPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, name, {values.size()}, {1});
    REQUIRE(arrayPtr != nullptr);
    FillArray(*arrayPtr, values);
    return DataPath({name});
  };
  const DataPath featureIds = createArray("FeatureIds", {1, 1, 1});
  const MultiArraySelectionParameter::ValueType inputs = {createArray("First", {0, 2, 3}), createArray("Second", {0, 1, 2}), createArray("Third", {0, 1, 1})};

  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIds));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputs));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histograms"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UserDefinedRange_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MinRange_Key, std::make_any<float64>(0.0));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaxRange_Key, std::make_any<float64>(2.0));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(2));

  std::vector<std::string> overflowMessages;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Info && message.message.find("not categorized into bin") != std::string::npos)
    {
      overflowMessages.push_back(message.message);
    }
  }};
  const auto result = scope.executeFilter(filter, dataStructure, args, nullptr, handler);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(overflowMessages.size() == 2);
  CHECK_THAT(overflowMessages[0], Catch::Matchers::StartsWith("2 ") && Catch::Matchers::EndsWith(" First"));
  CHECK_THAT(overflowMessages[1], Catch::Matchers::StartsWith("1 ") && Catch::Matchers::EndsWith(" Second"));
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Rejects invalid output shapes before execution", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();

  DataStructure dataStructure;
  auto* dataGroupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(dataGroupPtr != nullptr);
  auto* inputPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, dataGroupPtr->getId());
  auto* featureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {3}, {1}, dataGroupPtr->getId());
  auto* maskPtr = BoolArrayType::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {3}, {1}, dataGroupPtr->getId());
  REQUIRE(inputPtr != nullptr);
  REQUIRE(featureIdsPtr != nullptr);
  REQUIRE(maskPtr != nullptr);

  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input", "Values"})}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histograms"})));

  SECTION("Number of bins must be positive")
  {
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(0));
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.output_actions.invalid());
  }

  SECTION("FeatureIds and selected input must have compatible tuple counts")
  {
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.output_actions.invalid());
  }

  SECTION("Mask and selected input must have compatible tuple counts")
  {
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Mask"})));
    auto* matchingFeatureIdsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "MatchingFeatureIds", {4}, {1}, dataGroupPtr->getId());
    REQUIRE(matchingFeatureIdsPtr != nullptr);
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "MatchingFeatureIds"})));
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.output_actions.invalid());
  }
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Numeric-type sparse full and custom range parity", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  RunSparseTypeOracle<int8>(scope);
  RunSparseTypeOracle<uint8>(scope);
  RunSparseTypeOracle<int16>(scope);
  RunSparseTypeOracle<uint16>(scope);
  RunSparseTypeOracle<int32>(scope);
  RunSparseTypeOracle<uint32>(scope);
  RunSparseTypeOracle<int64>(scope);
  RunSparseTypeOracle<uint64>(scope);
  RunSparseTypeOracle<float32>(scope);
  RunSparseTypeOracle<float64>(scope);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline bounded modal fallback preserves exact raw-value modes", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  // Deliberately explicit: the normal in-core cache may be InCoreOnly, while this test must
  // exercise the no-external-sort bounded fallback on an in-memory store.
  unit_test::AlgorithmTestScope scope(*scanlineScenario);
  const std::vector<int32> oneFeature(4, 0);
  // Modes 1 and 2 occupy the two width-2 bins [0, 2] and [2, 4].
  CHECK(ExecuteInt32ModalCase(scope, {2, 1, 2, 1}, oneFeature, 2, 0.0, 4.0, true, DataPath({"TiedAscending"})) == std::vector<int32>{0, 2, 2, 4});
  CHECK(ExecuteInt32ModalCase(scope, {2, 1, 2, 1}, oneFeature, 1, 0.0, 4.0, true, DataPath({"DuplicatePairs"})) == std::vector<int32>{0, 4, 0, 4});
  std::vector<int32> uniqueValues(1024);
  std::iota(uniqueValues.begin(), uniqueValues.end(), 0);
  std::vector<int32> expectedUniqueRanges(uniqueValues.size() * 2);
  for(usize index = 0; index < expectedUniqueRanges.size(); index += 2)
  {
    expectedUniqueRanges[index] = 0;
    expectedUniqueRanges[index + 1] = static_cast<int32>(uniqueValues.size());
  }
  CHECK(ExecuteInt32ModalCase(scope, uniqueValues, std::vector<int32>(uniqueValues.size(), 0), 1, 0.0, static_cast<float64>(uniqueValues.size()), true, DataPath({"AllUniqueStress"})) ==
        expectedUniqueRanges);
  CHECK(ExecuteInt32ModalCase(scope, {9, 9, 1}, std::vector<int32>(3, 0), 2, 0.0, 4.0, true, DataPath({"OutsideMode"})).empty());
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: zero and negative FeatureIds are safe", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  CHECK(ExecuteInt32ModalCase(scope, {}, {}, 2, 0.0, 4.0, true, DataPath({"ZeroTuples"})).empty());
  CHECK(ExecuteInt32ModalCase(scope, {1, 2, 3}, {-1, -2, -3}, 2, 0.0, 4.0, true, DataPath({"AllNegative"})).empty());
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline rejects full-range integral maximum", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(groupPtr != nullptr);
  auto* valuesPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Values", {1}, {1}, groupPtr->getId());
  auto* idsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {1}, {1}, groupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  valuesPtr->getDataStoreRef()[0] = std::numeric_limits<uint8>::max();
  idsPtr->getDataStoreRef()[0] = 0;
  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input", "Values"})}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histogram"})));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  unit_test::AlgorithmTestScope scope(*scanlineScenario);
  const auto result = scope.executeFilter(filter, dataStructure, args);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().front().code == -23808);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: entry cancellation preserves facade outputs", "[SimplnxCore][ComputeArrayHistogram]")
{
  DataStructure dataStructure;
  auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
  auto* outputGroupPtr = DataGroup::Create(dataStructure, "Output");
  REQUIRE(inputGroupPtr != nullptr);
  REQUIRE(outputGroupPtr != nullptr);
  auto* valuesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {1}, {1}, inputGroupPtr->getId());
  auto* idsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {1}, {1}, inputGroupPtr->getId());
  auto* rangesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroupPtr->getId());
  auto* countsPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroupPtr->getId());
  auto* mostPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(rangesPtr != nullptr);
  REQUIRE(countsPtr != nullptr);
  REQUIRE(mostPtr != nullptr);
  rangesPtr->fill(11);
  countsPtr->fill(12);
  mostPtr->fill(13);
  std::atomic_bool shouldCancel = true;
  ComputeArrayHistogramByFeatureInputValues inputValues;
  inputValues.number_of_bins = 2;
  inputValues.selected_array_paths = {DataPath({"Input", "Values"})};
  inputValues.feature_ids_array_path = DataPath({"Input", "FeatureIds"});
  inputValues.created_bin_range_data_paths = {DataPath({"Output", "Ranges"})};
  inputValues.created_histogram_counts_data_paths = {DataPath({"Output", "Counts"})};
  inputValues.created_bin_most_populated_data_paths = {DataPath({"Output", "Most"})};
  const auto executeResult = ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  std::array<int32, 4> rangeValues = {};
  std::array<uint64, 2> countValues = {};
  std::array<uint64, 2> mostValues = {};
  auto rangesReadResult = rangesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangeValues.data(), rangeValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(rangesReadResult);
  auto countsReadResult = countsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countValues.data(), countValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsReadResult);
  auto mostReadResult = mostPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostValues.data(), mostValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(mostReadResult);
  REQUIRE(rangeValues == std::array<int32, 4>{11, 11, 11, 11});
  REQUIRE(countValues == std::array<uint64, 2>{12, 12});
  REQUIRE(mostValues == std::array<uint64, 2>{13, 13});
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline propagates bulk store failures", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  constexpr int32 k_ReadError = -923801;
  constexpr int32 k_WriteError = -923802;

  SECTION("input bulk read")
  {
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputGroupPtr = DataGroup::Create(dataStructure, "Output");
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputGroupPtr != nullptr);
    auto inputStore = std::make_shared<FailingReadDataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1}, k_ReadError);
    auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{0});
    auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", inputStore, inputGroupPtr->getId());
    auto* idsPtr = Int32ArrayType::Create(dataStructure, "FeatureIds", featureStore, inputGroupPtr->getId());
    auto* rangesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroupPtr->getId());
    auto* countsPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroupPtr->getId());
    auto* mostPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(idsPtr != nullptr);
    REQUIRE(rangesPtr != nullptr);
    REQUIRE(countsPtr != nullptr);
    REQUIRE(mostPtr != nullptr);

    ComputeArrayHistogramByFeatureInputValues inputValues;
    inputValues.user_defined_range = true;
    inputValues.number_of_bins = 2;
    inputValues.min_range = 0.0;
    inputValues.max_range = 2.0;
    inputValues.selected_array_paths = {DataPath({"Input", "Values"})};
    inputValues.feature_ids_array_path = DataPath({"Input", "FeatureIds"});
    inputValues.created_bin_range_data_paths = {DataPath({"Output", "Ranges"})};
    inputValues.created_histogram_counts_data_paths = {DataPath({"Output", "Counts"})};
    inputValues.created_bin_most_populated_data_paths = {DataPath({"Output", "Most"})};
    const std::atomic_bool shouldCancel = false;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_ReadError);
  }

  SECTION("numeric output bulk write")
  {
    DataStructure dataStructure;
    auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
    auto* outputGroupPtr = DataGroup::Create(dataStructure, "Output");
    REQUIRE(inputGroupPtr != nullptr);
    REQUIRE(outputGroupPtr != nullptr);
    auto inputStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1});
    auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{0});
    auto countStore = std::make_shared<FailingWriteDataStore<uint64>>(ShapeType{1}, ShapeType{2}, std::optional<uint64>{0}, k_WriteError);
    auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", inputStore, inputGroupPtr->getId());
    auto* idsPtr = Int32ArrayType::Create(dataStructure, "FeatureIds", featureStore, inputGroupPtr->getId());
    auto* rangesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroupPtr->getId());
    auto* countsPtr = UInt64ArrayType::Create(dataStructure, "Counts", countStore, outputGroupPtr->getId());
    auto* mostPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroupPtr->getId());
    REQUIRE(valuesPtr != nullptr);
    REQUIRE(idsPtr != nullptr);
    REQUIRE(rangesPtr != nullptr);
    REQUIRE(countsPtr != nullptr);
    REQUIRE(mostPtr != nullptr);

    ComputeArrayHistogramByFeatureInputValues inputValues;
    inputValues.user_defined_range = true;
    inputValues.number_of_bins = 2;
    inputValues.min_range = 0.0;
    inputValues.max_range = 2.0;
    inputValues.selected_array_paths = {DataPath({"Input", "Values"})};
    inputValues.feature_ids_array_path = DataPath({"Input", "FeatureIds"});
    inputValues.created_bin_range_data_paths = {DataPath({"Output", "Ranges"})};
    inputValues.created_histogram_counts_data_paths = {DataPath({"Output", "Counts"})};
    inputValues.created_bin_most_populated_data_paths = {DataPath({"Output", "Most"})};
    const std::atomic_bool shouldCancel = false;
    unit_test::AlgorithmTestScope scope(*scanlineScenario);
    const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: mid-scan cancellation preserves unwritten outputs", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  const auto scanlineScenario = SelectScanlineInMemoryScenario();
  if(!scanlineScenario.has_value())
  {
    SUCCEED("The scanline transfer contract is omitted by the InCoreOnly selection.");
    return;
  }
  constexpr usize k_TupleCount = 65537;
  std::atomic_bool shouldCancel = false;
  DataStructure dataStructure;
  auto* inputGroupPtr = DataGroup::Create(dataStructure, "Input");
  auto* outputGroupPtr = DataGroup::Create(dataStructure, "Output");
  REQUIRE(inputGroupPtr != nullptr);
  REQUIRE(outputGroupPtr != nullptr);
  auto inputStore = std::make_shared<CancelAfterReadDataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{1}, shouldCancel);
  auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{0});
  auto* valuesPtr = Int32ArrayType::Create(dataStructure, "Values", inputStore, inputGroupPtr->getId());
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "FeatureIds", featureStore, inputGroupPtr->getId());
  auto* rangesPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroupPtr->getId());
  auto* countsPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroupPtr->getId());
  auto* mostPtr = UInt64ArrayType::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(rangesPtr != nullptr);
  REQUIRE(countsPtr != nullptr);
  REQUIRE(mostPtr != nullptr);
  rangesPtr->fill(21);
  countsPtr->fill(22);
  mostPtr->fill(23);

  ComputeArrayHistogramByFeatureInputValues inputValues;
  inputValues.user_defined_range = true;
  inputValues.number_of_bins = 2;
  inputValues.min_range = 0.0;
  inputValues.max_range = 2.0;
  inputValues.selected_array_paths = {DataPath({"Input", "Values"})};
  inputValues.feature_ids_array_path = DataPath({"Input", "FeatureIds"});
  inputValues.created_bin_range_data_paths = {DataPath({"Output", "Ranges"})};
  inputValues.created_histogram_counts_data_paths = {DataPath({"Output", "Counts"})};
  inputValues.created_bin_most_populated_data_paths = {DataPath({"Output", "Most"})};
  unit_test::AlgorithmTestScope scope(*scanlineScenario);
  const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  REQUIRE(shouldCancel.load());
  std::array<int32, 4> rangeValues = {};
  std::array<uint64, 2> countValues = {};
  std::array<uint64, 2> mostValues = {};
  auto rangesReadResult = rangesPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangeValues.data(), rangeValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(rangesReadResult);
  auto countsReadResult = countsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countValues.data(), countValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(countsReadResult);
  auto mostReadResult = mostPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostValues.data(), mostValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(mostReadResult);
  REQUIRE(rangeValues == std::array<int32, 4>{21, 21, 21, 21});
  REQUIRE(countValues == std::array<uint64, 2>{22, 22});
  REQUIRE(mostValues == std::array<uint64, 2>{23, 23});
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: degenerate custom range modal oracle", "[SimplnxCore][ComputeArrayHistogram]")
{
  unit_test::LoadPlugins();
  constexpr usize k_FeatureCount = 1024;
  const std::vector<int32> values(k_FeatureCount, 7);
  std::vector<int32> featureIds(k_FeatureCount);
  std::iota(featureIds.begin(), featureIds.end(), 0);
  const auto scenarios = unit_test::SelectAlgorithmTestScenariosForInMemoryStores();
  for(const auto scenario : scenarios)
  {
    CAPTURE(scenario);
    unit_test::AlgorithmTestScope scope(scenario);
    std::vector<uint64> counts;
    std::vector<int32> ranges;
    std::vector<std::vector<int32>> modalLists;
    const auto modalRange = ExecuteInt32ModalCase(scope, values, featureIds, 2, 7.0, 7.0, true, DataPath({"Degenerate"}), &counts, &ranges, &modalLists);
    // The user supplied the single degenerate bin [7, 7] for every feature.
    CHECK(modalRange == std::vector<int32>{7, 7});
    for(const auto& modalList : modalLists)
    {
      CHECK(modalList == std::vector<int32>{7, 7});
    }
    REQUIRE(counts.size() == k_FeatureCount * 2);
    REQUIRE(counts[0] == 1);
    REQUIRE(counts[1] == 0);
    REQUIRE(counts.back() == 0);
    REQUIRE(ranges.size() == k_FeatureCount * 4);
    REQUIRE(std::all_of(ranges.begin(), ranges.end(), [](int32 value) { return value == 7; }));
  }
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: degenerate data range modal oracle", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Input");
  REQUIRE(groupPtr != nullptr);
  auto* valuesPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "Values", {4}, {1}, groupPtr->getId());
  auto* idsPtr = Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {4}, {1}, groupPtr->getId());
  REQUIRE(valuesPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  // At 2^25 and 2^26, float32 addition of 1 rounds back to the same value.
  // Each feature therefore has a literal, data-derived degenerate range.
  const std::array<float32, 2> expected = {33554432.0F, 67108864.0F};
  for(usize index = 0; index < 4; ++index)
  {
    valuesPtr->setValue(index, expected[index / 2]);
    idsPtr->setValue(index, static_cast<int32>(index / 2));
  }
  ComputeArrayHistogramByFeatureFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input", "Values"})}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histograms"})));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath output({"Histograms", "\"Values\" Histogram"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<float32>>(output.createChildPath("Modal Bin Ranges")));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(output.createChildPath("Bin Ranges")));
  const auto& modal = dataStructure.getDataRefAs<NeighborList<float32>>(output.createChildPath("Modal Bin Ranges"));
  const auto& ranges = dataStructure.getDataRefAs<Float32ArrayType>(output.createChildPath("Bin Ranges"));
  REQUIRE(modal.getNumberOfTuples() == 2);
  for(usize feature = 0; feature < 2; ++feature)
  {
    CHECK(modal.getList(feature) == std::vector<float32>{expected[feature], expected[feature]});
    for(usize component = 0; component < 4; ++component)
    {
      CHECK(ranges[feature * 4 + component] == expected[feature]);
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Modal ranges use paired bounds in later bins", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  const std::vector<int32> featureIds(4, 0);
  // Width-2 bins over [0, 8) put mode 5 in [4, 6] and mode 7 in [6, 8].
  CHECK(ExecuteInt32ModalCase(scope, {0, 5, 5, 7}, featureIds, 4, 0.0, 8.0, true, DataPath({"LaterBin"})) == std::vector<int32>{4, 6});
  CHECK(ExecuteInt32ModalCase(scope, {0, 7, 7, 5}, featureIds, 4, 0.0, 8.0, true, DataPath({"LastBin"})) == std::vector<int32>{6, 8});
  // The tied modes 1 and 2 occupy [0, 2] and [2, 4] in ascending value order.
  CHECK(ExecuteInt32ModalCase(scope, {2, 1, 2, 1}, featureIds, 2, 0.0, 4.0, true, DataPath({"TiedBins"})) == std::vector<int32>{0, 2, 2, 4});
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Modal ranges use paired bin offsets", "[SimplnxCore][ComputeArrayHistogramByFeature]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  std::vector<uint64> counts;
  std::vector<int32> ranges;
  std::vector<std::vector<int32>> modal;
  ExecuteInt32ModalCase(scope, {2}, {1}, 2, 0.0, 4.0, true, DataPath({"PairedOffsets"}), &counts, &ranges, &modal);
  REQUIRE(counts.size() == 4);
  REQUIRE(ranges.size() == 8);
  REQUIRE(modal.size() == 2);
  // Width (4 - 0) / 2 = 2 puts value 2 in bin 1: one count and bounds [2,4).
  CHECK(std::vector<uint64>(counts.begin() + 2, counts.end()) == std::vector<uint64>{0, 1});
  CHECK(std::vector<int32>(ranges.begin() + 4, ranges.end()) == std::vector<int32>{0, 2, 2, 4});
  CHECK(modal[1] == std::vector<int32>{2, 4});
}

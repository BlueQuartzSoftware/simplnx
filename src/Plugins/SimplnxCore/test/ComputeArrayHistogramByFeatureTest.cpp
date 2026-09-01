#include "SimplnxCore/Filters/Algorithms/ComputeArrayHistogramByFeature.hpp"
#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
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
constexpr float64 k_max_difference = 0.0001;
constexpr StringLiteral k_BinRangesName = "Ranges";
constexpr StringLiteral k_BinCountsName = "Counts";
constexpr StringLiteral k_Array0Name = "array0";
constexpr StringLiteral k_Array1Name = "array1";
constexpr StringLiteral k_Array2Name = "array2";

template <typename T>
class FailingReadDataStore : public DataStore<T>
{
public:
  FailingReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
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

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
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
void compareHistograms(const AbstractDataStore<T>& calculated, const std::array<T, N>& actual)
{
  if(calculated.getSize() != actual.size())
  {
    throw std::runtime_error(fmt::format("Improper sizing of DataStore. {} vs {}", calculated.getSize(), actual.size()));
  }
  for(int32 i = 0; i < N; i++)
  {
    T diff = std::fabs(calculated[i] - actual[i]);
    REQUIRE(diff < ::k_max_difference);
  }
}

template <typename T>
void fillArray(DataArray<T>& data, const std::vector<T>& values)
{
  int32 count = 0;
  for(T value : values)
  {
    data.getDataStore()->setValue(count, value);
    count++;
  }
}

template <typename T>
void runSparseTypeOracle(UnitTest::AlgorithmTestScope& scope)
{
  const DataPath inputPath({"Input", "Values"});
  const DataPath featurePath({"Input", "FeatureIds"});
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "Input");
  REQUIRE(group != nullptr);
  auto* input = DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, "Values", {8}, {1}, group->getId());
  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {8}, {1}, group->getId());
  REQUIRE(input != nullptr);
  REQUIRE(featureIds != nullptr);
  const std::array<T, 8> values = {T{0}, T{1}, T{2}, T{3}, T{0}, T{1}, T{2}, T{3}};
  const std::array<int32, 8> features = {0, 0, 2, 2, 4, 4, -1, -1};
  SIMPLNX_RESULT_REQUIRE_VALID(input->getDataStoreRef().copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(features.data(), features.size())));
  scope.requireExpectedStore(*input);
  scope.requireExpectedStore(*featureIds);

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
    const auto& counts = dataStructure.getDataRefAs<UInt64Array>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Bin Counts"));
    std::array<uint64, 20> actual = {};
    SIMPLNX_RESULT_REQUIRE_VALID(counts.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(actual.data(), actual.size())));
    const auto& mostPopulated = dataStructure.getDataRefAs<UInt64Array>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Most Populated Bin"));
    std::array<uint64, 10> mostPopulatedValues = {};
    SIMPLNX_RESULT_REQUIRE_VALID(mostPopulated.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostPopulatedValues.data(), mostPopulatedValues.size())));
    REQUIRE(mostPopulatedValues[0] == 0);
    REQUIRE(mostPopulatedValues[1] == 1);
    REQUIRE(mostPopulatedValues[2] == 0);
    REQUIRE(mostPopulatedValues[3] == 0);
    const auto& ranges = dataStructure.getDataRefAs<DataArray<T>>(groupPath.createChildPath("\"Values\" Histogram").createChildPath("Bin Ranges"));
    std::array<T, 40> rangeValues = {};
    SIMPLNX_RESULT_REQUIRE_VALID(ranges.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(rangeValues.data(), rangeValues.size())));
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

std::vector<int32> executeInt32ModalCase(UnitTest::AlgorithmTestScope& scope, const std::vector<int32>& values, const std::vector<int32>& featureIds, int32 bins, float64 minimum, float64 maximum,
                                         bool customRange, const DataPath& outputPath, std::vector<uint64>* countsResult = nullptr, std::vector<int32>* rangesResult = nullptr,
                                         std::vector<std::vector<int32>>* modalResults = nullptr)
{
  const DataPath inputPath({"Input", "Values"});
  const DataPath featurePath({"Input", "FeatureIds"});
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  REQUIRE(inputGroup != nullptr);
  auto* input = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {values.size()}, {1}, inputGroup->getId());
  auto* ids = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {featureIds.size()}, {1}, inputGroup->getId());
  REQUIRE(input != nullptr);
  REQUIRE(ids != nullptr);
  SIMPLNX_RESULT_REQUIRE_VALID(input->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(values.data(), values.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(ids->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size())));
  scope.requireExpectedStore(*input);
  scope.requireExpectedStore(*ids);

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto execute = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  const DataPath histogramPath = outputPath.createChildPath("\"Values\" Histogram");
  if(countsResult != nullptr)
  {
    const auto& counts = dataStructure.getDataRefAs<UInt64Array>(histogramPath.createChildPath("Bin Counts"));
    countsResult->resize(counts.getDataStoreRef().getSize());
    SIMPLNX_RESULT_REQUIRE_VALID(counts.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countsResult->data(), countsResult->size())));
  }
  if(rangesResult != nullptr)
  {
    const auto& ranges = dataStructure.getDataRefAs<Int32Array>(histogramPath.createChildPath("Bin Ranges"));
    rangesResult->resize(ranges.getDataStoreRef().getSize());
    SIMPLNX_RESULT_REQUIRE_VALID(ranges.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangesResult->data(), rangesResult->size())));
  }
  const auto& modal = dataStructure.getDataRefAs<NeighborList<int32>>(histogramPath.createChildPath("Modal Bin Ranges"));
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
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath histogramsDataPath({"Histograms"});
  std::string inputArrayName = "InputArray";
  DataPath inputArrayPath({"TestData", inputArrayName});
  DataPath featureIdsArrayPath({"TestData", "FeatureIds"});
  DataPath maskArrayPath({"TestData", "Mask"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, inputArrayName, {12}, {1}, topLevelGroup->getId());
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath histogramPath = histogramsDataPath.createChildPath(fmt::format("\"{}\" Histogram", inputArrayName));
    auto* dataGroup = dataStructure.getDataAs<DataGroup>(histogramPath);
    REQUIRE(dataGroup != nullptr);
    auto* histArray = dataStructure.getDataAs<UInt64Array>(histogramPath.createChildPath(histogram));
    REQUIRE(histArray != nullptr);
    REQUIRE(histArray->getNumberOfTuples() == 3);
    REQUIRE(histArray->getNumberOfComponents() == 5);
    auto* rangesArray = dataStructure.getDataAs<Int32Array>(histogramPath.createChildPath(ranges));
    REQUIRE(rangesArray != nullptr);
    REQUIRE(rangesArray->getNumberOfTuples() == 3);
    REQUIRE(rangesArray->getNumberOfComponents() == 10);
    auto* mostPopulatedBinArray = dataStructure.getDataAs<UInt64Array>(histogramPath.createChildPath(mostPopulatedBin));
    REQUIRE(mostPopulatedBinArray != nullptr);
    REQUIRE(mostPopulatedBinArray->getNumberOfTuples() == 3);
    auto* modalBinRangesArray = dataStructure.getDataAs<NeighborList<int32>>(histogramPath.createChildPath(modalBinRanges));
    REQUIRE(modalBinRangesArray != nullptr);
    REQUIRE(modalBinRangesArray->getNumberOfTuples() == 3);

    auto modalBinRange0 = (*modalBinRangesArray).getList(0);
    auto modalBinRange1 = (*modalBinRangesArray).getList(1);
    auto modalBinRange2 = (*modalBinRangesArray).getList(2);

    REQUIRE((*histArray)[0] == 1);
    REQUIRE((*histArray)[1] == 0);
    REQUIRE((*histArray)[2] == 0);
    REQUIRE((*histArray)[3] == 0);
    REQUIRE((*histArray)[4] == 1);
    REQUIRE((*histArray)[5] == 1);
    REQUIRE((*histArray)[6] == 0);
    REQUIRE((*histArray)[7] == 0);
    REQUIRE((*histArray)[8] == 1);
    REQUIRE((*histArray)[9] == 2);
    REQUIRE((*histArray)[10] == 2);
    REQUIRE((*histArray)[11] == 0);
    REQUIRE((*histArray)[12] == 0);
    REQUIRE((*histArray)[13] == 0);
    REQUIRE((*histArray)[14] == 2);

    REQUIRE((*rangesArray)[0] == 1);
    REQUIRE((*rangesArray)[1] == 15);
    REQUIRE((*rangesArray)[2] == 15);
    REQUIRE((*rangesArray)[3] == 30);
    REQUIRE((*rangesArray)[4] == 30);
    REQUIRE((*rangesArray)[5] == 44);
    REQUIRE((*rangesArray)[6] == 44);
    REQUIRE((*rangesArray)[7] == 59);
    REQUIRE((*rangesArray)[8] == 59);
    REQUIRE((*rangesArray)[9] == 74);
    REQUIRE((*rangesArray)[10] == 5);
    REQUIRE((*rangesArray)[11] == 8);
    REQUIRE((*rangesArray)[12] == 8);
    REQUIRE((*rangesArray)[13] == 11);
    REQUIRE((*rangesArray)[14] == 11);
    REQUIRE((*rangesArray)[15] == 14);
    REQUIRE((*rangesArray)[16] == 14);
    REQUIRE((*rangesArray)[17] == 17);
    REQUIRE((*rangesArray)[18] == 17);
    REQUIRE((*rangesArray)[19] == 21);
    REQUIRE((*rangesArray)[20] == 10);
    REQUIRE((*rangesArray)[21] == 12);
    REQUIRE((*rangesArray)[22] == 12);
    REQUIRE((*rangesArray)[23] == 15);
    REQUIRE((*rangesArray)[24] == 15);
    REQUIRE((*rangesArray)[25] == 17);
    REQUIRE((*rangesArray)[26] == 17);
    REQUIRE((*rangesArray)[27] == 20);
    REQUIRE((*rangesArray)[28] == 20);
    REQUIRE((*rangesArray)[29] == 23);

    REQUIRE((*mostPopulatedBinArray)[0] == 0);
    REQUIRE((*mostPopulatedBinArray)[1] == 1);
    REQUIRE((*mostPopulatedBinArray)[2] == 4);
    REQUIRE((*mostPopulatedBinArray)[3] == 2);
    REQUIRE((*mostPopulatedBinArray)[4] == 0);
    REQUIRE((*mostPopulatedBinArray)[5] == 2);

    REQUIRE(modalBinRange0[0] == 1);
    REQUIRE(modalBinRange0[1] == 15);
    REQUIRE(modalBinRange0[2] == 30);
    REQUIRE(modalBinRange0[3] == 44);
    REQUIRE(modalBinRange1[0] == 11);
    REQUIRE(modalBinRange1[1] == 14);
    REQUIRE(modalBinRange2[0] == 10);
    REQUIRE(modalBinRange2[1] == 12);
    REQUIRE(modalBinRange2[2] == 15);
    REQUIRE(modalBinRange2[3] == 17);
  }

  // No mask keeps the Scanline path on bulk reads without a mask buffer.
  {
    const DataPath noModalHistogramsPath({"HistogramsNoModal"});
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(noModalHistogramsPath));

    ComputeArrayHistogramByFeatureFilter filter;
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath histogramPath = noModalHistogramsPath.createChildPath(fmt::format("\"{}\" Histogram", inputArrayName)).createChildPath(histogram);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt64Array>(histogramPath));
    const auto& histogramArray = dataStructure.getDataRefAs<UInt64Array>(histogramPath);
    REQUIRE(histogramArray.getDataStoreRef()[0] == 1);
    REQUIRE(histogramArray.getDataStoreRef()[4] == 1);
    REQUIRE(histogramArray.getDataStoreRef()[9] == 2);
    REQUIRE(histogramArray.getDataStoreRef()[10] == 2);
    REQUIRE(histogramArray.getDataStoreRef()[14] == 3);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Rejects invalid output shapes before execution", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* dataGroup = DataGroup::Create(dataStructure, "Input");
  REQUIRE(dataGroup != nullptr);
  auto* input = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {4}, {1}, dataGroup->getId());
  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {3}, {1}, dataGroup->getId());
  auto* mask = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {3}, {1}, dataGroup->getId());
  REQUIRE(input != nullptr);
  REQUIRE(featureIds != nullptr);
  REQUIRE(mask != nullptr);

  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input", "Values"})}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histograms"})));

  SECTION("Number of bins must be positive")
  {
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(0));
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }

  SECTION("FeatureIds and selected input must have compatible tuple counts")
  {
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }

  SECTION("Mask and selected input must have compatible tuple counts")
  {
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "Mask"})));
    auto* matchingFeatureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "MatchingFeatureIds", {4}, {1}, dataGroup->getId());
    REQUIRE(matchingFeatureIds != nullptr);
    args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "MatchingFeatureIds"})));
    const auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Numeric-type sparse full and custom range parity", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  runSparseTypeOracle<int8>(scope);
  runSparseTypeOracle<uint8>(scope);
  runSparseTypeOracle<int16>(scope);
  runSparseTypeOracle<uint16>(scope);
  runSparseTypeOracle<int32>(scope);
  runSparseTypeOracle<uint32>(scope);
  runSparseTypeOracle<int64>(scope);
  runSparseTypeOracle<uint64>(scope);
  runSparseTypeOracle<float32>(scope);
  runSparseTypeOracle<float64>(scope);
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 1
TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline bounded modal fallback preserves exact raw-value modes", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  // Deliberately explicit: the normal in-core cache may be InCoreOnly, while this test must
  // exercise the no-external-sort bounded fallback on an in-memory store.
  UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  const std::vector<int32> oneFeature(4, 0);
  CHECK(executeInt32ModalCase(scope, {2, 1, 2, 1}, oneFeature, 2, 0.0, 4.0, true, DataPath({"TiedAscending"})) == std::vector<int32>{0, 2, 2, 2});
  CHECK(executeInt32ModalCase(scope, {2, 1, 2, 1}, oneFeature, 1, 0.0, 4.0, true, DataPath({"DuplicatePairs"})) == std::vector<int32>{0, 4, 0, 4});
  std::vector<int32> uniqueValues(1024);
  std::iota(uniqueValues.begin(), uniqueValues.end(), 0);
  std::vector<int32> expectedUniqueRanges(uniqueValues.size() * 2);
  for(usize index = 0; index < expectedUniqueRanges.size(); index += 2)
  {
    expectedUniqueRanges[index] = 0;
    expectedUniqueRanges[index + 1] = static_cast<int32>(uniqueValues.size());
  }
  CHECK(executeInt32ModalCase(scope, uniqueValues, std::vector<int32>(uniqueValues.size(), 0), 1, 0.0, static_cast<float64>(uniqueValues.size()), true, DataPath({"AllUniqueStress"})) ==
        expectedUniqueRanges);
  CHECK(executeInt32ModalCase(scope, {9, 9, 1}, std::vector<int32>(3, 0), 2, 0.0, 4.0, true, DataPath({"OutsideMode"})).empty());
}
#endif

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: zero and negative FeatureIds are safe", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  CHECK(executeInt32ModalCase(scope, {}, {}, 2, 0.0, 4.0, true, DataPath({"ZeroTuples"})).empty());
  CHECK(executeInt32ModalCase(scope, {1, 2, 3}, {-1, -2, -3}, 2, 0.0, 4.0, true, DataPath({"AllNegative"})).empty());
}

#if SIMPLNX_TEST_ALGORITHM_PATH != 1
TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline rejects full-range integral maximum", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure;
  auto* group = DataGroup::Create(dataStructure, "Input");
  REQUIRE(group != nullptr);
  auto* values = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Values", {1}, {1}, group->getId());
  auto* ids = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {1}, {1}, group->getId());
  REQUIRE(values != nullptr);
  REQUIRE(ids != nullptr);
  values->getDataStoreRef()[0] = std::numeric_limits<uint8>::max();
  ids->getDataStoreRef()[0] = 0;
  ComputeArrayHistogramByFeatureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NumberOfBins_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input", "Values"})}));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Input", "FeatureIds"})));
  args.insertOrAssign(ComputeArrayHistogramByFeatureFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histogram"})));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  const auto result = scope.executeFilter(filter, dataStructure, args);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().front().code == -23808);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: entry cancellation preserves facade outputs", "[SimplnxCore][ComputeArrayHistogram]")
{
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  auto* outputGroup = DataGroup::Create(dataStructure, "Output");
  REQUIRE(inputGroup != nullptr);
  REQUIRE(outputGroup != nullptr);
  auto* values = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Values", {1}, {1}, inputGroup->getId());
  auto* ids = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {1}, {1}, inputGroup->getId());
  auto* ranges = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroup->getId());
  auto* counts = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroup->getId());
  auto* most = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroup->getId());
  REQUIRE(values != nullptr);
  REQUIRE(ids != nullptr);
  REQUIRE(ranges != nullptr);
  REQUIRE(counts != nullptr);
  REQUIRE(most != nullptr);
  ranges->fill(11);
  counts->fill(12);
  most->fill(13);
  std::atomic_bool shouldCancel = true;
  ComputeArrayHistogramByFeatureInputValues inputValues;
  inputValues.NumberOfBins = 2;
  inputValues.SelectedArrayPaths = {DataPath({"Input", "Values"})};
  inputValues.FeatureIdsArrayPath = DataPath({"Input", "FeatureIds"});
  inputValues.CreatedBinRangeDataPaths = {DataPath({"Output", "Ranges"})};
  inputValues.CreatedHistogramCountsDataPaths = {DataPath({"Output", "Counts"})};
  inputValues.CreatedBinMostPopulatedDataPaths = {DataPath({"Output", "Most"})};
  const auto executeResult = ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  std::array<int32, 4> rangeValues = {};
  std::array<uint64, 2> countValues = {};
  std::array<uint64, 2> mostValues = {};
  SIMPLNX_RESULT_REQUIRE_VALID(ranges->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangeValues.data(), rangeValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(counts->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countValues.data(), countValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(most->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostValues.data(), mostValues.size())));
  REQUIRE(rangeValues == std::array<int32, 4>{11, 11, 11, 11});
  REQUIRE(countValues == std::array<uint64, 2>{12, 12});
  REQUIRE(mostValues == std::array<uint64, 2>{13, 13});
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: Scanline propagates bulk store failures", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  constexpr int32 k_ReadError = -923801;
  constexpr int32 k_WriteError = -923802;

  SECTION("input bulk read")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* outputGroup = DataGroup::Create(dataStructure, "Output");
    REQUIRE(inputGroup != nullptr);
    REQUIRE(outputGroup != nullptr);
    auto inputStore = std::make_shared<FailingReadDataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1}, k_ReadError);
    auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{0});
    auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
    auto* ids = Int32Array::Create(dataStructure, "FeatureIds", featureStore, inputGroup->getId());
    auto* ranges = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroup->getId());
    auto* counts = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroup->getId());
    auto* most = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroup->getId());
    REQUIRE(values != nullptr);
    REQUIRE(ids != nullptr);
    REQUIRE(ranges != nullptr);
    REQUIRE(counts != nullptr);
    REQUIRE(most != nullptr);

    ComputeArrayHistogramByFeatureInputValues inputValues;
    inputValues.UserDefinedRange = true;
    inputValues.NumberOfBins = 2;
    inputValues.MinRange = 0.0;
    inputValues.MaxRange = 2.0;
    inputValues.SelectedArrayPaths = {DataPath({"Input", "Values"})};
    inputValues.FeatureIdsArrayPath = DataPath({"Input", "FeatureIds"});
    inputValues.CreatedBinRangeDataPaths = {DataPath({"Output", "Ranges"})};
    inputValues.CreatedHistogramCountsDataPaths = {DataPath({"Output", "Counts"})};
    inputValues.CreatedBinMostPopulatedDataPaths = {DataPath({"Output", "Most"})};
    const std::atomic_bool shouldCancel = false;
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_ReadError);
  }

  SECTION("numeric output bulk write")
  {
    DataStructure dataStructure;
    auto* inputGroup = DataGroup::Create(dataStructure, "Input");
    auto* outputGroup = DataGroup::Create(dataStructure, "Output");
    REQUIRE(inputGroup != nullptr);
    REQUIRE(outputGroup != nullptr);
    auto inputStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{1});
    auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{4}, ShapeType{1}, std::optional<int32>{0});
    auto countStore = std::make_shared<FailingWriteDataStore<uint64>>(ShapeType{1}, ShapeType{2}, std::optional<uint64>{0}, k_WriteError);
    auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
    auto* ids = Int32Array::Create(dataStructure, "FeatureIds", featureStore, inputGroup->getId());
    auto* ranges = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroup->getId());
    auto* counts = UInt64Array::Create(dataStructure, "Counts", countStore, outputGroup->getId());
    auto* most = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroup->getId());
    REQUIRE(values != nullptr);
    REQUIRE(ids != nullptr);
    REQUIRE(ranges != nullptr);
    REQUIRE(counts != nullptr);
    REQUIRE(most != nullptr);

    ComputeArrayHistogramByFeatureInputValues inputValues;
    inputValues.UserDefinedRange = true;
    inputValues.NumberOfBins = 2;
    inputValues.MinRange = 0.0;
    inputValues.MaxRange = 2.0;
    inputValues.SelectedArrayPaths = {DataPath({"Input", "Values"})};
    inputValues.FeatureIdsArrayPath = DataPath({"Input", "FeatureIds"});
    inputValues.CreatedBinRangeDataPaths = {DataPath({"Output", "Ranges"})};
    inputValues.CreatedHistogramCountsDataPaths = {DataPath({"Output", "Counts"})};
    inputValues.CreatedBinMostPopulatedDataPaths = {DataPath({"Output", "Most"})};
    const std::atomic_bool shouldCancel = false;
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: mid-scan cancellation preserves unwritten outputs", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_TupleCount = 65537;
  std::atomic_bool shouldCancel = false;
  DataStructure dataStructure;
  auto* inputGroup = DataGroup::Create(dataStructure, "Input");
  auto* outputGroup = DataGroup::Create(dataStructure, "Output");
  REQUIRE(inputGroup != nullptr);
  REQUIRE(outputGroup != nullptr);
  auto inputStore = std::make_shared<CancelAfterReadDataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{1}, shouldCancel);
  auto featureStore = std::make_shared<DataStore<int32>>(ShapeType{k_TupleCount}, ShapeType{1}, std::optional<int32>{0});
  auto* values = Int32Array::Create(dataStructure, "Values", inputStore, inputGroup->getId());
  auto* ids = Int32Array::Create(dataStructure, "FeatureIds", featureStore, inputGroup->getId());
  auto* ranges = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Ranges", {1}, {4}, outputGroup->getId());
  auto* counts = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Counts", {1}, {2}, outputGroup->getId());
  auto* most = UInt64Array::CreateWithStore<DataStore<uint64>>(dataStructure, "Most", {1}, {2}, outputGroup->getId());
  REQUIRE(values != nullptr);
  REQUIRE(ids != nullptr);
  REQUIRE(ranges != nullptr);
  REQUIRE(counts != nullptr);
  REQUIRE(most != nullptr);
  ranges->fill(21);
  counts->fill(22);
  most->fill(23);

  ComputeArrayHistogramByFeatureInputValues inputValues;
  inputValues.UserDefinedRange = true;
  inputValues.NumberOfBins = 2;
  inputValues.MinRange = 0.0;
  inputValues.MaxRange = 2.0;
  inputValues.SelectedArrayPaths = {DataPath({"Input", "Values"})};
  inputValues.FeatureIdsArrayPath = DataPath({"Input", "FeatureIds"});
  inputValues.CreatedBinRangeDataPaths = {DataPath({"Output", "Ranges"})};
  inputValues.CreatedHistogramCountsDataPaths = {DataPath({"Output", "Counts"})};
  inputValues.CreatedBinMostPopulatedDataPaths = {DataPath({"Output", "Most"})};
  UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  const auto executeResult = scope.execute([&] { return ComputeArrayHistogramByFeature(dataStructure, {}, shouldCancel, &inputValues)(); });
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  REQUIRE(shouldCancel.load());
  std::array<int32, 4> rangeValues = {};
  std::array<uint64, 2> countValues = {};
  std::array<uint64, 2> mostValues = {};
  SIMPLNX_RESULT_REQUIRE_VALID(ranges->getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(rangeValues.data(), rangeValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(counts->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(countValues.data(), countValues.size())));
  SIMPLNX_RESULT_REQUIRE_VALID(most->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint64>(mostValues.data(), mostValues.size())));
  REQUIRE(rangeValues == std::array<int32, 4>{21, 21, 21, 21});
  REQUIRE(countValues == std::array<uint64, 2>{22, 22});
  REQUIRE(mostValues == std::array<uint64, 2>{23, 23});
}
#endif

#if SIMPLNX_TEST_ALGORITHM_PATH != 1
TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: degenerate custom range modal oracle", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_FeatureCount = 1024;
  const std::vector<int32> values(k_FeatureCount, 7);
  std::vector<int32> featureIds(k_FeatureCount);
  std::iota(featureIds.begin(), featureIds.end(), 0);
  UnitTest::AlgorithmTestScope directScope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
  std::vector<uint64> directCounts;
  std::vector<int32> directRanges;
  std::vector<std::vector<int32>> directModalLists;
  const auto direct = executeInt32ModalCase(directScope, values, featureIds, 2, 7.0, 7.0, true, DataPath({"DegenerateDirect"}), &directCounts, &directRanges, &directModalLists);
  UnitTest::AlgorithmTestScope scanlineScope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  std::vector<uint64> scanlineCounts;
  std::vector<int32> scanlineRanges;
  const auto scanline = executeInt32ModalCase(scanlineScope, values, featureIds, 2, 7.0, 7.0, true, DataPath({"DegenerateScanline"}), &scanlineCounts, &scanlineRanges);
  // Every populated feature must use the complete range [0, numFeatures).
  REQUIRE(direct == std::vector<int32>{0, 1024});
  REQUIRE(directModalLists[512] == direct);
  REQUIRE(directModalLists[1023] == direct);
  REQUIRE(scanline == direct);
  REQUIRE(directCounts.size() == k_FeatureCount * 2);
  REQUIRE(directCounts[0] == 1);
  REQUIRE(directCounts[1] == 0);
  REQUIRE(directCounts.back() == 0);
  REQUIRE(scanlineCounts == directCounts);
  REQUIRE(directRanges.size() == k_FeatureCount * 4);
  REQUIRE(std::all_of(directRanges.begin(), directRanges.end(), [](int32 value) { return value == 7; }));
  REQUIRE(scanlineRanges == directRanges);
}
#endif

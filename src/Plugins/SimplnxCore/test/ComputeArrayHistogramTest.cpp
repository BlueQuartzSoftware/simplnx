#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
constexpr float64 k_max_difference = 0.0001;
constexpr StringLiteral k_BinRangesName = "Ranges";
constexpr StringLiteral k_BinCountsName = "Counts";
constexpr StringLiteral k_Array0Name = "array0";
constexpr StringLiteral k_Array1Name = "array1";
constexpr StringLiteral k_Array2Name = "array2";

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
} // namespace

TEST_CASE("SimplnxCore::ComputeArrayHistogram: Counts & Bins Only", "[SimplnxCore][ComputeArrayHistogram]")
{
  UnitTest::LoadPlugins();

  // Configure the filter arguments.
  ComputeArrayHistogramFilter filter;
  DataStructure dataStruct;
  Arguments args;

  // load vector with data paths for test
  ::fillArray(*DataArray<float64>::CreateWithStore<DataStore<float64>>(dataStruct, k_Array0Name, {static_cast<usize>(4)}, {static_cast<usize>(3)}),
              {0.0, 5.5, 8.5, 9.2, 16.7, 907.3, 5.0, 6.9, 83.7387483, -56.8, 3.7, -4.9});
  ::fillArray(*DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStruct, k_Array1Name, {static_cast<usize>(4)}, {static_cast<usize>(3)}), {56, 82, 46, 93, 73, 57, 24, 32, -90, -35, 74, -19});
  ::fillArray(*DataArray<uint32>::CreateWithStore<DataStore<uint32>>(dataStruct, k_Array2Name, {static_cast<usize>(4)}, {static_cast<usize>(3)}), {83, 93, 75, 67, 8977, 56, 48, 92, 57, 34, 34, 34});

  std::vector<DataPath> dataPaths = dataStruct.getAllDataPaths();
  auto parentPath = dataPaths[0].getParent();
  auto dataGPath = parentPath.createChildPath("HistogramDataGroup");

  args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(4));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(dataPaths));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(dataGPath));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(std::string{::k_BinRangesName}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinCountName_Key, std::make_any<std::string>(std::string{::k_BinCountsName}));

  auto preflightResult = filter.preflight(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  {
    std::array<float64, 8> binRangesSet = {-56.8, 184.475, 184.475, 425.75, 425.75, 667.025, 667.025, 908.3};
    std::array<uint64, 4> binCountsSet = {11, 0, 0, 1};
    const DataPath histogramPath = dataGPath.createChildPath(fmt::format("\"{}\" Histogram", k_Array0Name));

    compareHistograms(dataStruct.getDataAs<Float64Array>(histogramPath.createChildPath(std::string{k_BinRangesName}))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(histogramPath.createChildPath(std::string{k_BinCountsName}))->getDataStoreRef(), binCountsSet);
  }
  {
    std::array<int32, 8> binRangesSet = {-90, -44, -44, 2, 2, 48, 48, 94};
    std::array<uint64, 4> binCountsSet = {1, 2, 3, 6};
    const DataPath histogramPath = dataGPath.createChildPath(fmt::format("\"{}\" Histogram", k_Array1Name));

    compareHistograms(dataStruct.getDataAs<Int32Array>(histogramPath.createChildPath(std::string{k_BinRangesName}))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(histogramPath.createChildPath(std::string{k_BinCountsName}))->getDataStoreRef(), binCountsSet);
  }
  {
    std::array<uint32, 8> binRangesSet = {34, 2270, 2270, 4506, 4506, 6742, 6742, 8978};
    std::array<uint64, 4> binCountsSet = {11, 0, 0, 1};
    const DataPath histogramPath = dataGPath.createChildPath(fmt::format("\"{}\" Histogram", k_Array2Name));

    compareHistograms(dataStruct.getDataAs<UInt32Array>(histogramPath.createChildPath(std::string{k_BinRangesName}))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(histogramPath.createChildPath(std::string{k_BinCountsName}))->getDataStoreRef(), binCountsSet);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStruct);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogram: All Histogram Calculations", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath histogramsDataPath({"Histograms"});
  std::string inputArrayName = "InputArray";
  DataPath inputArrayPath({"TestData", inputArrayName});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, inputArrayName, {11}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 20;
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

  const std::string histogram = "Histogram";
  const std::string ranges = "Ranges";
  const std::string mostPopulatedBin = "Most Populated Bin";
  const std::string modalBinRanges = "Modal Bin Ranges";

  // Execute the configured filter.
  {
    ComputeArrayHistogramFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayHistogramFilter::k_MinRange_Key, std::make_any<float64>(0));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_MaxRange_Key, std::make_any<float64>(100));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(5));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({inputArrayPath}));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(histogramsDataPath));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinCountName_Key, std::make_any<std::string>(histogram));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(ranges));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoMostPopulatedBinName_Key, std::make_any<std::string>(mostPopulatedBin));
    args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoModalBinRangesName_Key, std::make_any<std::string>(modalBinRanges));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    DataPath histogramPath = histogramsDataPath.createChildPath(fmt::format("\"{}\" Histogram", inputArrayName));
    auto* dataGroup = dataStructure.getDataAs<DataGroup>(histogramPath);
    REQUIRE(dataGroup != nullptr);
    auto* histArray = dataStructure.getDataAs<UInt64Array>(histogramPath.createChildPath(histogram));
    REQUIRE(histArray != nullptr);
    auto* rangesArray = dataStructure.getDataAs<Int32Array>(histogramPath.createChildPath(ranges));
    REQUIRE(rangesArray != nullptr);
    auto* mostPopulatedBinArray = dataStructure.getDataAs<UInt64Array>(histogramPath.createChildPath(mostPopulatedBin));
    REQUIRE(mostPopulatedBinArray != nullptr);
    auto* modalBinRangesArray = dataStructure.getDataAs<NeighborList<int32>>(histogramPath.createChildPath(modalBinRanges));
    REQUIRE(modalBinRangesArray != nullptr);

    auto modalBinRangesVals = (*modalBinRangesArray).getList(0);

    REQUIRE(modalBinRangesVals.size() == 2);
    REQUIRE(modalBinRangesVals[0] == 1);
    REQUIRE(modalBinRangesVals[1] == 6);

    REQUIRE(histArray->size() == 5);
    REQUIRE((*histArray)[0] == 4);
    REQUIRE((*histArray)[1] == 2);
    REQUIRE((*histArray)[2] == 2);
    REQUIRE((*histArray)[3] == 0);
    REQUIRE((*histArray)[4] == 1);

    REQUIRE(rangesArray->size() == 10);
    REQUIRE((*rangesArray)[0] == 1);
    REQUIRE((*rangesArray)[1] == 10);
    REQUIRE((*rangesArray)[2] == 10);
    REQUIRE((*rangesArray)[3] == 19);
    REQUIRE((*rangesArray)[4] == 19);
    REQUIRE((*rangesArray)[5] == 28);
    REQUIRE((*rangesArray)[6] == 28);
    REQUIRE((*rangesArray)[7] == 37);
    REQUIRE((*rangesArray)[8] == 37);
    REQUIRE((*rangesArray)[9] == 46);

    REQUIRE(mostPopulatedBinArray->size() == 2);
    REQUIRE((*mostPopulatedBinArray)[0] == 0);
    REQUIRE((*mostPopulatedBinArray)[1] == 4);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogram: Masked Bulk Reads Cross Chunk Boundary", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  constexpr usize k_NumTuples = 65538;
  DataStructure dataStructure;
  auto* inputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {k_NumTuples}, {1});
  auto* maskArray = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {k_NumTuples}, {1});
  auto& inputStore = inputArray->getDataStoreRef();
  auto& maskStore = maskArray->getDataStoreRef();
  std::fill(inputStore.begin(), inputStore.end(), int32{0});
  std::fill(maskStore.begin(), maskStore.end(), uint8{1});
  inputStore[65535] = 4;
  inputStore[65536] = 8;
  inputStore[65537] = 100;
  maskStore[65537] = 0;

  const DataPath histogramGroupPath({"Histograms"});
  ComputeArrayHistogramFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input"})}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(histogramGroupPath));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoModalBinRangesName_Key, std::make_any<std::string>("Modal Ranges"));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(std::string{::k_BinRangesName}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinCountName_Key, std::make_any<std::string>(std::string{::k_BinCountsName}));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath = histogramGroupPath.createChildPath("\"Input\" Histogram");
  const Int32Array* ranges = nullptr;
  REQUIRE_NOTHROW(ranges = &dataStructure.getDataRefAs<Int32Array>(outputPath.createChildPath(std::string{k_BinRangesName})));
  const UInt64Array* counts = nullptr;
  REQUIRE_NOTHROW(counts = &dataStructure.getDataRefAs<UInt64Array>(outputPath.createChildPath(std::string{k_BinCountsName})));
  compareHistograms(ranges->getDataStoreRef(), std::array<int32, 4>{0, 4, 4, 9});
  compareHistograms(counts->getDataStoreRef(), std::array<uint64, 2>{k_NumTuples - 2, 1});
  const NeighborList<int32>* modalRanges = nullptr;
  REQUIRE_NOTHROW(modalRanges = &dataStructure.getDataRefAs<NeighborList<int32>>(outputPath.createChildPath("Modal Ranges")));
  const std::vector<int32> expectedModalRanges = {0, 3};
  REQUIRE(modalRanges->getList(0) == expectedModalRanges);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogram: All Distinct Modal Values", "[SimplnxCore][ComputeArrayHistogram]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  auto* inputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {1});
  inputArray->getDataStoreRef()[0] = 5;
  inputArray->getDataStoreRef()[1] = 1;
  inputArray->getDataStoreRef()[2] = 3;

  const DataPath histogramGroupPath({"Histograms"});
  ComputeArrayHistogramFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(2));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input"})}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(histogramGroupPath));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoModalBinRangesName_Key, std::make_any<std::string>("Modal Ranges"));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath modalPath = histogramGroupPath.createChildPath("\"Input\" Histogram").createChildPath("Modal Ranges");
  const NeighborList<int32>* modalRanges = nullptr;
  REQUIRE_NOTHROW(modalRanges = &dataStructure.getDataRefAs<NeighborList<int32>>(modalPath));
  const std::vector<int32> expectedModalRanges = {1, 2, 3, 4, 0, 0};
  REQUIRE(modalRanges->getList(0) == expectedModalRanges);
}

TEST_CASE("SimplnxCore::ComputeArrayHistogramFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeArrayHistogramFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeArrayHistogramFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeArrayHistogramFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeArrayHistogramFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ComputeArrayHistogramFilter::k_NumberOfBins_Key) == 5);
      CHECK(args.value<bool>(ComputeArrayHistogramFilter::k_UserDefinedRange_Key) == true);
      CHECK(args.value<float64>(ComputeArrayHistogramFilter::k_MinRange_Key) == 2.5);
      CHECK(args.value<float64>(ComputeArrayHistogramFilter::k_MaxRange_Key) == 2.5);
      CHECK(args.value<bool>(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key) == true);
      // Complex type (SingleToMultiDataPathSelectionFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ComputeArrayHistogramFilter::k_HistoBinCountName_Key) == "TestName");
    }
  }
}

namespace
{
class CancelAfterFirstReadStore : public DataStore<int32>
{
public:
  explicit CancelAfterFirstReadStore(std::atomic_bool& cancelled)
  : DataStore<int32>({21846}, {3}, 0)
  , m_Cancelled(cancelled)
  {
  }

  ~CancelAfterFirstReadStore() override = default;
  CancelAfterFirstReadStore(const CancelAfterFirstReadStore&) = delete;
  CancelAfterFirstReadStore(CancelAfterFirstReadStore&&) = delete;
  CancelAfterFirstReadStore& operator=(const CancelAfterFirstReadStore&) = delete;
  CancelAfterFirstReadStore& operator=(CancelAfterFirstReadStore&&) = delete;

  [[nodiscard]] Result<> copyIntoBuffer(usize startIndex, nonstd::span<int32> buffer) const override
  {
    Result<> result = DataStore<int32>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_Reads.emplace_back(startIndex, buffer.size());
      m_Cancelled.store(true);
    }
    return result;
  }

  const std::vector<std::pair<usize, usize>>& reads() const
  {
    return m_Reads;
  }

private:
  std::atomic_bool& m_Cancelled;
  mutable std::vector<std::pair<usize, usize>> m_Reads;
};

Arguments c02HistogramArgs(bool customRange = true)
{
  const ComputeArrayHistogramFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(4));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(customRange));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_MinRange_Key, std::make_any<float64>(0));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_MaxRange_Key, std::make_any<float64>(8));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(DataPath({"Histograms"})));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input"})}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CalculateModalBinRanges_Key, std::make_any<bool>(true));
  return args;
}

template <typename T, usize N>
void c02CheckExact(const AbstractDataStore<T>& store, const std::array<T, N>& expected)
{
  REQUIRE(store.getSize() == expected.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(store[index] == expected.at(index));
  }
}

void c02CheckCounts(const DataStructure& dataStructure, const std::array<uint64, 4>& expected)
{
  const UInt64Array* countsPtr = nullptr;
  REQUIRE_NOTHROW(countsPtr = &dataStructure.getDataRefAs<UInt64Array>(DataPath({"Histograms", "\"Input\" Histogram", "Bin Counts"})));
  REQUIRE(countsPtr != nullptr);
  c02CheckExact(countsPtr->getDataStoreRef(), expected);
}
} // namespace

TEST_CASE("SimplnxCore::C02 histogram tuple mask safe RED", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {3});
  REQUIRE(inputPtr != nullptr);
  fillArray(*inputPtr, std::vector<int32>{0, 2, 4, 100, 102, 104, 4, 6, 6});
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {3}, {1});
  REQUIRE(maskPtr != nullptr);
  fillArray(*maskPtr, std::vector<uint8>{2, 0, 255});
  scope.requireExpectedStore(*inputPtr);
  scope.requireExpectedStore(*maskPtr);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  c02CheckCounts(dataStructure, {1, 1, 2, 2});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram tuple masks full outputs", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool boolMask = GENERATE(false, true);
  const bool customRange = GENERATE(false, true);
  CAPTURE(boolMask, customRange);
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {3});
  REQUIRE(inputPtr != nullptr);
  fillArray(*inputPtr, std::vector<int32>{0, 2, 4, 100, 102, 104, 4, 6, 6});
  if(boolMask)
  {
    auto* maskPtr = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {3}, {1});
    REQUIRE(maskPtr != nullptr);
    (*maskPtr)[0] = true;
    (*maskPtr)[1] = false;
    (*maskPtr)[2] = true;
    scope.requireExpectedStore(*maskPtr);
  }
  else
  {
    auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {3}, {1});
    REQUIRE(maskPtr != nullptr);
    fillArray(*maskPtr, std::vector<uint8>{2, 0, 255});
    scope.requireExpectedStore(*maskPtr);
  }
  scope.requireExpectedStore(*inputPtr);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs(customRange);
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  c02CheckCounts(dataStructure, {1, 1, 2, 2});
  const DataPath output({"Histograms", "\"Input\" Histogram"});
  const Int32Array* rangesPtr = nullptr;
  const UInt64Array* mostPtr = nullptr;
  const NeighborList<int32>* modesPtr = nullptr;
  REQUIRE_NOTHROW(rangesPtr = &dataStructure.getDataRefAs<Int32Array>(output.createChildPath("Bin Ranges")));
  REQUIRE_NOTHROW(mostPtr = &dataStructure.getDataRefAs<UInt64Array>(output.createChildPath("Most Populated Bin")));
  REQUIRE_NOTHROW(modesPtr = &dataStructure.getDataRefAs<NeighborList<int32>>(output.createChildPath("Modal Bin Ranges")));
  REQUIRE(rangesPtr != nullptr);
  REQUIRE(mostPtr != nullptr);
  REQUIRE(modesPtr != nullptr);
  if(customRange)
  {
    c02CheckExact(rangesPtr->getDataStoreRef(), std::array<int32, 8>{0, 2, 2, 4, 4, 6, 6, 8});
  }
  else
  {
    c02CheckExact(rangesPtr->getDataStoreRef(), std::array<int32, 8>{0, 1, 1, 3, 3, 5, 5, 7});
  }
  c02CheckExact(mostPtr->getDataStoreRef(), std::array<uint64, 2>{2, 2});
  const std::vector<int32> expectedModes = {4, 5, 6, 7};
  REQUIRE(modesPtr->getList(0) == expectedModes);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram real OOC tuple masks", "[SimplnxCore][ComputeArrayHistogram][.OocStoreContract]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const bool maskOnlyDisk = GENERATE(false, true);
  CAPTURE(maskOnlyDisk);
  DataStructure dataStructure;
  Int32Array* inputPtr = nullptr;
  if(maskOnlyDisk)
  {
    inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {3});
  }
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  if(!maskOnlyDisk)
  {
    auto store = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Input"}), {3}, {3});
    inputPtr = Int32Array::Create(dataStructure, "Input", store);
  }
  auto maskStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, DataPath({"Mask"}), {3}, {1});
  auto* maskPtr = UInt8Array::Create(dataStructure, "Mask", maskStore);
  REQUIRE(inputPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  REQUIRE((inputPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC") == !maskOnlyDisk);
  REQUIRE(maskPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  fillArray(*inputPtr, std::vector<int32>{0, 2, 4, 100, 102, 104, 4, 6, 6});
  fillArray(*maskPtr, std::vector<uint8>{2, 0, 255});
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto before = GetAlgorithmPathExecutionCounts();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  c02CheckCounts(dataStructure, {1, 1, 2, 2});
  const NeighborList<int32>* modesPtr = nullptr;
  REQUIRE_NOTHROW(modesPtr = &dataStructure.getDataRefAs<NeighborList<int32>>(DataPath({"Histograms", "\"Input\" Histogram", "Modal Bin Ranges"})));
  REQUIRE(modesPtr != nullptr);
  const std::vector<int32> expectedModes = {4, 5, 6, 7};
  REQUIRE(modesPtr->getList(0) == expectedModes);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram tuple masks cross value pages", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  const bool wideTuple = GENERATE(false, true);
  CAPTURE(wideTuple);
  const usize tuples = wideTuple ? 1 : 21846;
  const usize components = wideTuple ? 65537 : 3;
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {tuples}, {components});
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {tuples}, {1});
  REQUIRE(inputPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  inputPtr->fill(0);
  maskPtr->fill(1);
  if(wideTuple)
  {
    (*inputPtr)[0] = 2;
    (*inputPtr)[65535] = 4;
    (*inputPtr)[65536] = 6;
  }
  else
  {
    (*inputPtr)[65535] = 4;
    (*inputPtr)[65536] = 6;
    (*inputPtr)[65537] = 6;
  }
  scope.requireExpectedStore(*inputPtr);
  scope.requireExpectedStore(*maskPtr);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  if(wideTuple)
  {
    c02CheckCounts(dataStructure, {65534, 1, 1, 1});
  }
  else
  {
    c02CheckCounts(dataStructure, {65535, 0, 1, 2});
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram all masked automatic range", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {3});
  REQUIRE(inputPtr != nullptr);
  inputPtr->fill(2);
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {3}, {1});
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(0);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs(false);
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().front().code == -23766);
}

TEST_CASE("SimplnxCore::C02 histogram mixed component arrays", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {3}, {3});
  auto* secondPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Second", {3}, {2});
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {3}, {1});
  REQUIRE(inputPtr != nullptr);
  REQUIRE(secondPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  fillArray(*inputPtr, std::vector<int32>{0, 2, 4, 100, 102, 104, 4, 6, 6});
  fillArray(*secondPtr, std::vector<int32>{0, 2, 100, 102, 4, 6});
  fillArray(*maskPtr, std::vector<uint8>{2, 0, 255});
  scope.requireExpectedStore(*inputPtr);
  scope.requireExpectedStore(*secondPtr);
  scope.requireExpectedStore(*maskPtr);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({DataPath({"Input"}), DataPath({"Second"})}));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  c02CheckCounts(dataStructure, {1, 1, 2, 2});
  const UInt64Array* countsPtr = nullptr;
  REQUIRE_NOTHROW(countsPtr = &dataStructure.getDataRefAs<UInt64Array>(DataPath({"Histograms", "\"Second\" Histogram", "Bin Counts"})));
  REQUIRE(countsPtr != nullptr);
  c02CheckExact(countsPtr->getDataStoreRef(), std::array<uint64, 4>{1, 1, 1, 1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram HDF5 tuple straddles page", "[SimplnxCore][ComputeArrayHistogram][.OocStoreContract]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  DataStructure dataStructure;
  auto inputStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, DataPath({"Input"}), {21846}, {3});
  auto maskStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, DataPath({"Mask"}), {21846}, {1});
  auto* inputPtr = Int32Array::Create(dataStructure, "Input", inputStore);
  auto* maskPtr = UInt8Array::Create(dataStructure, "Mask", maskStore);
  REQUIRE(inputPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  REQUIRE(inputPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  REQUIRE(maskPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  inputPtr->fill(0);
  maskPtr->fill(0);
  (*maskPtr)[21845] = 2;
  (*inputPtr)[65535] = 2;
  (*inputPtr)[65536] = 4;
  (*inputPtr)[65537] = 6;
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto before = GetAlgorithmPathExecutionCounts();
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto after = GetAlgorithmPathExecutionCounts();
  REQUIRE(after.OutOfCoreOnOutOfCoreStore == before.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(after.InCore == before.InCore);
  c02CheckCounts(dataStructure, {0, 1, 1, 1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::C02 histogram cancellation between value pages", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  std::atomic_bool cancelled = false;
  DataStructure dataStructure;
  auto inputStore = std::make_shared<CancelAfterFirstReadStore>(cancelled);
  auto* inputPtr = Int32Array::Create(dataStructure, "Input", inputStore);
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {21846}, {1});
  REQUIRE(inputPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  maskPtr->fill(1);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs();
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  REQUIRE(inputStore->reads().empty());
  REQUIRE_FALSE(cancelled.load());
  auto result = filter.execute(dataStructure, args, nullptr, {}, cancelled);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().front().code == -1);
  REQUIRE(cancelled.load());
  REQUIRE(inputStore->reads().size() == 1);
  REQUIRE(inputStore->reads().front().first == 0);
  REQUIRE(inputStore->reads().front().second == 65536);
}

TEST_CASE("SimplnxCore::C02 histogram empty automatic range", "[SimplnxCore][ComputeArrayHistogram][C02]")
{
  const auto appPtr = Application::GetOrCreateInstance();
  REQUIRE(appPtr != nullptr);
  DataStructure dataStructure;
  auto* inputPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "Input", {0}, {3});
  auto* maskPtr = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Mask", {0}, {1});
  REQUIRE(inputPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  const ComputeArrayHistogramFilter filter;
  auto args = c02HistogramArgs(false);
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  REQUIRE(result.result.invalid());
  REQUIRE(result.result.errors().front().code == -23766);
}

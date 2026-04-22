#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

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

  // Instantiate the filter, a DataStructure object and an Arguments Object
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

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(4));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_CreateNewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(dataPaths));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_NewDataGroupPath_Key, std::make_any<DataPath>(dataGPath));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(std::string{::k_BinRangesName}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinCountName_Key, std::make_any<std::string>(std::string{::k_BinCountsName}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
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

  // Execute the Find Array Statistics Filter
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
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

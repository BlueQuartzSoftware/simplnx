#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

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

TEST_CASE("SimplnxCore::ComputeArrayHistogramByFeature: All Histogram Calculations", "[SimplnxCore][ComputeArrayHistogram]")
{
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

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayHistogramByFeatureFilter filter;
    Arguments args;

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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    UnitTest::WriteTestDataStructure(dataStructure, "/tmp/ds.dream3d");
  }

  // Check resulting values
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
}

#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"

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
constexpr StringLiteral k_BinRangesSuffix = " Ranges";
constexpr StringLiteral k_BinCountsSuffix = " Counts";
constexpr StringLiteral k_Array0Name = "array0";
constexpr StringLiteral k_Array1Name = "array1";
constexpr StringLiteral k_Array2Name = "array2";

template <typename T, usize N>
void compareHistograms(const AbstractDataStore<T>& calculated, const std::array<T, N>& actual)
{
  if(calculated.getSize() != actual.size())
  {
    throw std::runtime_error("Improper sizing of DataStore");
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

TEST_CASE("SimplnxCore::ComputeArrayHistogram: Valid Filter Execution", "[SimplnxCore][ComputeArrayHistogram]")
{
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
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinRangeName_Key, std::make_any<std::string>(std::string{::k_BinRangesSuffix}));
  args.insertOrAssign(ComputeArrayHistogramFilter::k_HistoBinCountName_Key, std::make_any<std::string>(std::string{::k_BinCountsSuffix}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  {
    std::array<float64, 5> binRangesSet = {-56.8, 184.475, 425.75, 667.025, 908.3};
    std::array<uint64, 4> binCountsSet = {11, 0, 0, 1};
    const std::string name = k_Array0Name;

    compareHistograms(dataStruct.getDataAs<Float64Array>(dataGPath.createChildPath((name + std::string{k_BinRangesSuffix})))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(dataGPath.createChildPath((name + std::string{k_BinCountsSuffix})))->getDataStoreRef(), binCountsSet);
  }
  {
    std::array<int32, 5> binRangesSet = {-90, -44, 2, 48, 94};
    std::array<uint64, 4> binCountsSet = {1, 2, 3, 6};
    const std::string name = k_Array1Name;

    compareHistograms(dataStruct.getDataAs<Int32Array>(dataGPath.createChildPath((name + std::string{k_BinRangesSuffix})))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(dataGPath.createChildPath((name + std::string{k_BinCountsSuffix})))->getDataStoreRef(), binCountsSet);
  }
  {
    std::array<uint32, 5> binRangesSet = {34, 2270, 4506, 6742, 8978};
    std::array<uint64, 4> binCountsSet = {11, 0, 0, 1};
    const std::string name = k_Array2Name;

    compareHistograms(dataStruct.getDataAs<UInt32Array>(dataGPath.createChildPath((name + std::string{k_BinRangesSuffix})))->getDataStoreRef(), binRangesSet);
    compareHistograms(dataStruct.getDataAs<UInt64Array>(dataGPath.createChildPath((name + std::string{k_BinCountsSuffix})))->getDataStoreRef(), binCountsSet);
  }
}

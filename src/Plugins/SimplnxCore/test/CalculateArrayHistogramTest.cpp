#include "SimplnxCore/Filters/CalculateArrayHistogramFilter.hpp"

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

void compareHistograms(const DataArray<float64>& calulated, const std::array<float64, 8>& actual)
{
  for(int32 i = 0; i < actual.size(); i++)
  {
    float64 diff = std::fabs(calulated[i] - actual[i]);
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

TEST_CASE("SimplnxCore::CalculateArrayHistogram: Valid Filter Execution", "[SimplnxCore][CalculateArrayHistogram]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CalculateArrayHistogramFilter filter;
  DataStructure dataStruct;
  Arguments args;

  // load vector with data paths for test
  ::fillArray(*DataArray<float64>::CreateWithStore<DataStore<float64>>(dataStruct, "array0", {static_cast<usize>(4)}, {static_cast<usize>(3)}),
              {0.0, 5.5, 8.5, 9.2, 16.7, 907.3, 5.0, 6.9, 83.7387483, -56.8, 3.7, -4.9});
  ::fillArray(*DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStruct, "array1", {static_cast<usize>(4)}, {static_cast<usize>(3)}), {56, 82, 46, 93, 73, 57, 24, 32, -90, -35, 74, -19});
  ::fillArray(*DataArray<uint32>::CreateWithStore<DataStore<uint32>>(dataStruct, "array2", {static_cast<usize>(4)}, {static_cast<usize>(3)}), {83, 93, 75, 67, 8977, 56, 48, 92, 57, 34, 34, 34});

  std::vector<DataPath> dataPaths = dataStruct.getAllDataPaths();
  auto parentPath = dataPaths[0].getParent();
  auto dataGPath = parentPath.createChildPath("HistogramDataGroup");

  // Create default Parameters for the filter.
  args.insertOrAssign(CalculateArrayHistogramFilter::k_NumberOfBins_Key, std::make_any<int32>(4));
  args.insertOrAssign(CalculateArrayHistogramFilter::k_UserDefinedRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(CalculateArrayHistogramFilter::k_NewDataGroup_Key, std::make_any<bool>(true));
  args.insertOrAssign(CalculateArrayHistogramFilter::k_SelectedArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(dataPaths));
  args.insertOrAssign(CalculateArrayHistogramFilter::k_NewDataGroupName_Key, std::make_any<DataPath>(dataGPath));
  args.insertOrAssign(CalculateArrayHistogramFilter::k_HistoName_Key, std::make_any<std::string>("Histogram"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStruct, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // load vector with child paths from filter
  std::vector<DataPath> createdDataPaths;
  for(auto& selectedArrayPath : dataPaths) // regenerate based on preflight
  {
    const auto& dataArray = dataStruct.getDataAs<IDataArray>(selectedArrayPath);
    auto childPath = dataGPath.createChildPath((dataArray->getName() + "Histogram"));
    createdDataPaths.push_back(childPath);
  }

  std::array<float64, 8> array0HistogramSet = {183.725, 11, 425.25, 0, 666.775, 0, 908.3, 1};
  std::array<float64, 8> array1HistogramSet = {-44.75, 1, 1.5, 2, 47.75, 3, 94, 6};
  std::array<float64, 8> array2HistogramSet = {2269.25, 11, 4505.5, 0, 6741.75, 0, 8978, 1};
  for(const auto& child : createdDataPaths)
  {
    auto& dataArray = dataStruct.getDataRefAs<DataArray<float64>>(child);
    if(dataArray.getName().find("array0") != std::string::npos)
    {
      compareHistograms(dataArray, array0HistogramSet);
    }
    else if(dataArray.getName().find("array1") != std::string::npos)
    {
      compareHistograms(dataArray, array1HistogramSet);
    }
    else if(dataArray.getName().find("array2") != std::string::npos)
    {
      compareHistograms(dataArray, array2HistogramSet);
    }
  }
}

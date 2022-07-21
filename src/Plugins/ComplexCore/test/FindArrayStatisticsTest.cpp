#include <catch2/catch.hpp>

#include <filesystem>
namespace fs = std::filesystem;

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"
#include "ComplexCore/Filters/ImportDREAM3DFilter.hpp"

using namespace complex;
using namespace complex::Constants;

template <typename T>
void CompareDataArrays(const IDataArray& left, const IDataArray& right)
{
  const auto& oldDataStore = left.getIDataStoreRefAs<AbstractDataStore<T>>();
  const auto& newDataStore = right.getIDataStoreRefAs<AbstractDataStore<T>>();
  usize start = 0;
  usize end = oldDataStore.getSize();
  bool same = true;
  usize badIndex = 0;
  for(usize i = start; i < end; i++)
  {
    if(oldDataStore[i] != newDataStore[i])
    {
      badIndex = i;
      REQUIRE(oldDataStore[badIndex] == newDataStore[badIndex]);
      break;
    }
  }
}

TEST_CASE("FindArrayStatisticsFilter: Instantiate Filter", "[ComplexCore][FindArrayStatisticsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindArrayStatisticsFilter filter;
  DataStructure ds;
  Arguments args;

  DataPath inputArrayPath;
  DataPath destPath;
  DataPath maskArrayPath;
  DataPath featureIdsArrayPath;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindHistogram_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MinRange_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaxRange_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(FindArrayStatisticsFilter::k_UseFullRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_NumBins_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(false));
  // args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
  // args.insertOrAssign(FindArrayStatisticsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsArrayPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskArrayPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(destPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(""));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("FindArrayStatisticsFilter: Test Algorithm", "[ComplexCore][FindArrayStatisticsFilter]")
{
  const std::string k_StatisticsGroupName("Statistics");
  const std::string k_Mask("Mask");
  const std::string k_SEMSignal("SEM Signal");
  const std::string k_FeatureIdsName("FeatureIds");
  const DataPath k_TopLevelItemPath({"Exemplar Data"});
  const DataPath k_StatisticsGroupPath = k_TopLevelItemPath.createChildPath(k_StatisticsGroupName);
  const DataPath k_CellAttributeMatrix = k_TopLevelItemPath.createChildPath("CellData");
  const DataPath k_MaskArrayPath = k_CellAttributeMatrix.createChildPath(k_Mask);
  const DataPath k_FeatureIdsPath = k_CellAttributeMatrix.createChildPath(k_FeatureIdsName);
  const DataPath k_SEMSignalPath = k_CellAttributeMatrix.createChildPath(k_SEMSignal);

  DataStructure exemplarDataStructure;
  // Read Exemplar DREAM3D File Filter
  {
    ImportDREAM3DFilter filter;

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/find_array_statistics.dream3d", unit_test::k_DREAM3DDataDir));

    Arguments args;
    args.insertOrAssign(ImportDREAM3DFilter::k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(exemplarDataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(exemplarDataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  DataStructure dataStructure;
  // read data set
  {
    ImportDREAM3DFilter filter;

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/TestFiles/bad_data_neighbor_orientation_check.dream3d", unit_test::k_DREAM3DDataDir));

    Arguments args;
    args.insertOrAssign(ImportDREAM3DFilter::k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // create a data group to store the statistics
  DataGroup* statisticsData = DataGroup::Create(dataStructure, k_StatisticsGroupName, dataStructure.getData(k_TopLevelItemPath)->getId());

  // Find Array Statistics Filter
  {
    FindArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindHistogram_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinRange_Key, std::make_any<float64>(-1300.0));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaxRange_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseFullRange_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_NumBins_Key, std::make_any<int32>(8));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    // args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_SEMSignalPath));
    // args.insertOrAssign(FindArrayStatisticsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(k_StatisticsGroupPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>("Histogram"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>("Length"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>("Minimum"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>("Maximum"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>("Mean"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>("Median"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>("Standard Deviation"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>("Summation"));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>("Standardization"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Test Data / CellData' group
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<DataGroup>(k_CellAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      DataPath exemplarDataArrayPath(generatedPathVector);
      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(), type, exemplarType)
                  << std::endl;
        continue;
      }

      switch(type)
      {
      case DataType::boolean: {
        CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
  }
}
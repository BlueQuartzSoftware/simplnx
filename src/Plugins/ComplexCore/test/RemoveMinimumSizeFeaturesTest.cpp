#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RemoveMinimumSizeFeaturesFilter.hpp"

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;

TEST_CASE("ComplexCore::RemoveMinimumSizeFeatures: Small IN100 Pipeline", "[ComplexCore][RemoveMinimumSizeFeatures]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_min_size_output.tar.gz", "6_6_min_size_output.dream3d");

  const complex::UnitTest::TestFileSentinel testDataSentinel1(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_min_size_input.tar.gz", "6_6_min_size_input.dream3d");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_min_size_output.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_min_size_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  const std::string k_GrainData("Grain Data");
  const std::string k_NumCells("NumElements");
  const std::string k_ExemplarDataContainer("Exemplar Data");
  const DataPath k_DataContainerPath({Constants::k_DataContainer});
  const DataPath k_CellAttributeMatrix = k_DataContainerPath.createChildPath(Constants::k_CellData);
  const DataPath k_CellFeatureAttributeMatrix = k_DataContainerPath.createChildPath(k_GrainData);
  const DataPath k_FeatureIdsArrayPath = k_CellAttributeMatrix.createChildPath(Constants::k_FeatureIds);
  const DataPath k_NumCellsPath = k_CellFeatureAttributeMatrix.createChildPath(k_NumCells);
  const DataPath k_FeaturePhasesPath = k_CellFeatureAttributeMatrix.createChildPath(Constants::k_Phases);

  {
    RemoveMinimumSizeFeaturesFilter filter;

    // Parameter Keys
    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_ApplySinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_PhaseNumber_Key, std::make_any<int64>(1));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_NumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
    args.insertOrAssign(RemoveMinimumSizeFeaturesFilter::k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(k_CellFeatureAttributeMatrix);
    std::vector<DataPath> selectedCellArrays;

    // Create the vector of selected cell DataPaths
    for(auto& child : cellDataGroup)
    {
      selectedCellArrays.push_back(k_CellFeatureAttributeMatrix.createChildPath(child.second->getName()));
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = generatedDataArray.getDataType();

      // Now generate the path to the exemplar data set in the exemplar data structure.
      std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
      generatedPathVector[0] = k_ExemplarDataContainer;
      DataPath exemplarDataArrayPath(generatedPathVector);

      // Check to see if there is something to compare against in the exemplar file.
      if(nullptr == exemplarDataStructure.getDataAs<IDataArray>(exemplarDataArrayPath))
      {
        continue;
      }

      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      std::cout << "Comparing: " << cellArrayPath.toString() << " <==> " << exemplarDataArrayPath.toString() << std::endl;

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(), type, exemplarType)
                  << std::endl;
        continue;
      }

      switch(type)
      {
      case DataType::boolean: {
        UnitTest::CompareDataArrays<bool>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int8: {
        UnitTest::CompareDataArrays<int8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int16: {
        UnitTest::CompareDataArrays<int16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int32: {
        UnitTest::CompareDataArrays<int32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::int64: {
        UnitTest::CompareDataArrays<int64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint8: {
        UnitTest::CompareDataArrays<uint8>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint16: {
        UnitTest::CompareDataArrays<uint16>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint32: {
        UnitTest::CompareDataArrays<uint32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::uint64: {
        UnitTest::CompareDataArrays<uint64>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float32: {
        UnitTest::CompareDataArrays<float32>(generatedDataArray, exemplarDataArray);
        break;
      }
      case DataType::float64: {
        UnitTest::CompareDataArrays<float64>(generatedDataArray, exemplarDataArray);
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/7_0_min_size_output.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
}

#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "complex_plugins/Utilities/TestUtilities.hpp"

#include "Core/Core_test_dirs.hpp"

using namespace complex;

TEST_CASE("ComplexCore::RemoveMinimumSizeFeatures: Small IN100 Pipeline", "[ComplexCore][RemoveMinimumSizeFeatures]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/TestFiles/6_6_min_size_output.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_min_size_input.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  {
    auto filter = filterList->createFilter(k_RemoveMinimumSizeFeaturesFilterHandle);
    REQUIRE(nullptr != filter);

    // Parameter Keys
    constexpr StringLiteral k_MinAllowedFeaturesSize_Key = "min_allowed_features_size";

    constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_phases_path";
    constexpr StringLiteral k_NumCellsPath_Key = "num_cells_path";
    constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
    constexpr StringLiteral k_ImageGeomPath_Key = "image_geom_path";
    constexpr StringLiteral k_ApplySinglePhase_Key = "apply_single_phase";
    constexpr StringLiteral k_PhaseNumber_Key = "phase_number";

    Arguments args;
    // Create default Parameters for the filter.
    args.insertOrAssign(k_MinAllowedFeaturesSize_Key, std::make_any<int64>(16));
    args.insertOrAssign(k_ApplySinglePhase_Key, std::make_any<bool>(false));
    args.insertOrAssign(k_PhaseNumber_Key, std::make_any<int64>(1));
    args.insertOrAssign(k_ImageGeomPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(k_FeatureIdsPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(k_NumCellsPath_Key, std::make_any<DataPath>(k_NumCellsPath));
    args.insertOrAssign(k_FeaturePhasesPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
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

  WriteTestDataStructure(dataStructure, fmt::format("{}/7_0_min_size_output.dream3d", unit_test::k_BinaryTestOutputDir));
}

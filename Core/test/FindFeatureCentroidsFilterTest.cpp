#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindFeatureCentroidsFilter.hpp"
#include "CoreTestUtilities.hpp"

using namespace complex;

TEST_CASE("Core::FindFeatureCentroidsFilter", "[Core][FindFeatureCentroidsFilter]")
{
  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Make sure we can load the needed filters from the plugins
  const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");

  const Uuid k_ImportDream3dFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ImportDream3dFilterHandle(k_ImportDream3dFilterId, k_ComplexCorePluginId);

  const std::string k_Quats("Quats");
  const std::string k_Phases("Phases");
  const std::string k_Mask("Mask");
  const std::string k_FeatureIds("FeatureIds");
  const std::string k_DataContainer("Small IN100");
  const std::string k_SmallIN100ScanData("EBSD Scan Data");
  const std::string k_CellFeatureData("CellFeatureData");
  const std::string k_CentroidsNX("Centroids NX");
  const std::string k_Centroids("Centroids");

  DataStructure dataStructure;
  // Read Exemplar DREAM3D File Filter
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestDataSourceDir));

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Instantiate FindFeatureCentroidsFilter
  {
    FindFeatureCentroidsFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath({k_DataContainer, k_SmallIN100ScanData, k_FeatureIds});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});
    const DataPath k_FeatureAttributeMatrix({k_DataContainer, k_CellFeatureData});
    const DataPath k_SelectedImageGeometry({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindFeatureCentroidsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsNXArrayPath));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(k_FeatureAttributeMatrix));
    args.insertOrAssign(FindFeatureCentroidsFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedImageGeometry));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});
    const DataPath k_CentroidsNXArrayPath({k_DataContainer, k_CellFeatureData, k_CentroidsNX});

    const auto& k_CentroidsArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsArrayPath);
    const auto& k_CentroidsNXArray = dataStructure.getDataRefAs<IDataArray>(k_CentroidsNXArrayPath);

    CompareDataArrays<float>(k_CentroidsArray, k_CentroidsNXArray);
  }

  {
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/find_feature_centroids.dream3d", unit_test::k_BinaryTestOutputDir));
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}

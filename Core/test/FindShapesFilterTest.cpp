#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindShapesFilter.hpp"
#include "CoreTestUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("Core::FindShapesFilter", "[Core][FindShapesFilter]")
{

  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Make sure we can load the needed filters from the plugins
  const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");

  const Uuid k_ImportDream3dFilterId = *Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
  const FilterHandle k_ImportDream3dFilterHandle(k_ImportDream3dFilterId, k_ComplexCorePluginId);

  // Top Level DataGroup
  const std::string k_DataContainer("Small IN100");
  // Feature Level Data
  const std::string k_CellFeatureData("CellFeatureData");
  const std::string k_Centroids("Centroids");
  // Cell Level Group
  const std::string k_SmallIN100ScanData("EBSD Scan Data");
  const std::string k_FeatureIds("FeatureIds");

  //
  const std::string k_Omega3sArrayName("Omega3s");
  const std::string k_AxisLengthsArrayName("AxisLengths");
  const std::string k_AxisEulerAnglesArrayName("AxisEulerAngles");
  const std::string k_AspectRatiosArrayName("AspectRatios");
  const std::string k_VolumesArrayName("Volumes");

  DataStructure dataStructure;
  // Read Exemplar DREAM3D File Filter
  {
    constexpr StringLiteral k_ImportFileData = "Import_File_Data";

    auto filter = filterList->createFilter(k_ImportDream3dFilterHandle);
    REQUIRE(nullptr != filter);

    Dream3dImportParameter::ImportData parameter;
    parameter.FilePath = fs::path(fmt::format("{}/6_6_find_shapes.dream3d", unit_test::k_TestDataSourceDir));

    Arguments args;
    args.insertOrAssign(k_ImportFileData, std::make_any<Dream3dImportParameter::ImportData>(parameter));

    // Preflight the filter and check result
    auto preflightResult = filter->preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Instantiate FindShapesFilter
  {
    FindShapesFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath({k_DataContainer, k_SmallIN100ScanData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});

    const DataPath k_Omega3sArrayPath({k_DataContainer, k_CellFeatureData, k_Omega3sArrayName + "NX"});
    const DataPath k_AxisLengthsArrayPath({k_DataContainer, k_CellFeatureData, k_AxisLengthsArrayName + "NX"});
    const DataPath k_AxisEulerAnglesArrayPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayName + "NX"});
    const DataPath k_AspectRatiosArrayPath({k_DataContainer, k_CellFeatureData, k_AspectRatiosArrayName + "NX"});
    const DataPath k_VolumesArrayPath({k_DataContainer, k_CellFeatureData, k_VolumesArrayName + "NX"});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindShapesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(FindShapesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(k_CellFeatureAttributeMatrixPath));
    args.insertOrAssign(FindShapesFilter::k_CentroidsArrayPath_Key, std::make_any<DataPath>(k_CentroidsArrayPath));
    args.insertOrAssign(FindShapesFilter::k_Omega3sArrayName_Key, std::make_any<DataPath>(k_Omega3sArrayPath));
    args.insertOrAssign(FindShapesFilter::k_AxisLengthsArrayName_Key, std::make_any<DataPath>(k_AxisLengthsArrayPath));
    args.insertOrAssign(FindShapesFilter::k_AxisEulerAnglesArrayName_Key, std::make_any<DataPath>(k_AxisEulerAnglesArrayPath));
    args.insertOrAssign(FindShapesFilter::k_AspectRatiosArrayName_Key, std::make_any<DataPath>(k_AspectRatiosArrayPath));
    args.insertOrAssign(FindShapesFilter::k_VolumesArrayName_Key, std::make_any<DataPath>(k_VolumesArrayPath));
    args.insertOrAssign(FindShapesFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_Omega3sArrayName, k_AxisLengthsArrayName, k_AxisEulerAnglesArrayName, k_AspectRatiosArrayName, k_VolumesArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({k_DataContainer, k_CellFeatureData, comparisonName + "NX"});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      CompareDataArrays<float>(exemplarData, calculatedData);
    }
  }

  // Write the DataStructure out to the file system
  {
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/find_shapes.dream3d", unit_test::k_BinaryTestOutputDir));
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}

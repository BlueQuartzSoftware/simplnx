#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindShapesFilter.hpp"

#include "complex_plugins/Utilities/TestUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

TEST_CASE("Core::FindShapesFilter", "[Core][FindShapesFilter]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_find_shapes.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);

  const std::string k_Omega3sArrayName("Omega3s");
  const std::string k_AxisLengthsArrayName("AxisLengths");
  const std::string k_AxisEulerAnglesArrayName("AxisEulerAngles");
  const std::string k_AspectRatiosArrayName("AspectRatios");
  const std::string k_VolumesArrayName("Volumes");

  // Instantiate FindShapesFilter
  {
    FindShapesFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_SmallIN100ScanData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_CentroidsArrayPath({k_DataContainer, k_CellFeatureData, k_Centroids});

    const DataPath k_Omega3sArrayPath({k_DataContainer, k_CellFeatureData, k_Omega3sArrayName + "NX"});
    const DataPath k_AxisLengthsArrayPath({k_DataContainer, k_CellFeatureData, k_AxisLengthsArrayName + "NX"});
    const DataPath k_AxisEulerAnglesArrayPath({k_DataContainer, k_CellFeatureData, k_AxisEulerAnglesArrayName + "NX"});
    const DataPath k_AspectRatiosArrayPath({k_DataContainer, k_CellFeatureData, k_AspectRatiosArrayName + "NX"});
    const DataPath k_VolumesArrayPath({k_DataContainer, k_CellFeatureData, k_VolumesArrayName + "NX"});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindShapesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
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
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_shapes.dream3d", unit_test::k_BinaryDir)));
}


#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/FindSurfaceAreaToVolumeFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_SurfaceAreaVolumeRationArrayName("SurfaceAreaVolumeRatio");
const std::string k_SphericityArrayName("Sphericity");
} // namespace

TEST_CASE("Core::FindSurfaceAreaToVolume", "[Core][FindSurfaceAreaToVolume]")
{
  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/TestFiles/6_6_stats_test.dream3d", unit_test::k_DREAM3DDataDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  {
    FindSurfaceAreaToVolumeFilter filter;
    Arguments args;

    const DataPath k_FeatureIdsArrayPath2({k_DataContainer, k_CellData, k_FeatureIds});
    const DataPath k_CellFeatureAttributeMatrixPath({k_DataContainer, k_CellFeatureData});
    const DataPath k_NumElementsArrayPath({k_DataContainer, k_CellFeatureData, k_NumElements});
    const DataPath k_SelectedGeometryPath({k_DataContainer});

    // Create default Parameters for the filter.
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath2));
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_NumCellsArrayPath_Key, std::make_any<DataPath>(k_NumElementsArrayPath));
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(k_SelectedGeometryPath));
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_CalculateSphericity_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_SurfaceAreaVolumeRatioArrayName_Key, std::make_any<DataPath>({k_SurfaceAreaVolumeRationArrayName}));
    args.insertOrAssign(FindSurfaceAreaToVolumeFilter::k_SphericityArrayName_Key, std::make_any<DataPath>({k_SphericityArrayName}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Compare the output arrays with those precalculated from the file
  {
    std::vector<std::string> comparisonNames = {k_SurfaceAreaVolumeRationArrayName, k_SphericityArrayName};
    for(const auto& comparisonName : comparisonNames)
    {
      const DataPath exemplarPath({k_DataContainer, k_CellFeatureData, comparisonName});
      const DataPath calculatedPath({comparisonName});
      const auto& exemplarData = dataStructure.getDataRefAs<IDataArray>(exemplarPath);
      const auto& calculatedData = dataStructure.getDataRefAs<IDataArray>(calculatedPath);
      UnitTest::CompareDataArrays<float32>(exemplarData, calculatedData);
    }
  }

  // Write the DataStructure out to the file system
  UnitTest::WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_surface_area_volume_ratio.dream3d", unit_test::k_BinaryTestOutputDir)));
}

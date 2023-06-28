#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/SilhouetteFilter.hpp"

using namespace complex;

namespace
{
const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);

const DataPath k_MedoidsClusterIdsPath = k_CellPath.createChildPath("MedoidsClusterIds");
const DataPath k_MeansClusterIdsPath = k_CellPath.createChildPath("MeansClusterIds");

const std::string k_MedoidsSilhouetteName = "MedoidsSilhouette";
const std::string k_MeansSilhouetteName = "MeansSilhouette";

const DataPath k_MedoidsSilhouettePath = k_CellPath.createChildPath(k_MedoidsSilhouetteName);
const DataPath k_MeansSilhouettePath = k_CellPath.createChildPath(k_MeansSilhouetteName);

const DataPath k_MedoidsSilhouettePathNX = k_CellPath.createChildPath(k_MedoidsSilhouetteName + "NX");
const DataPath k_MeansSilhouettePathNX = k_CellPath.createChildPath(k_MeansSilhouetteName + "NX");
} // namespace

TEST_CASE("ComplexCore::SilhouetteFilter: Medoids Test", "[ComplexCore][SilhouetteFilter]")
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    SilhouetteFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MedoidsClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MedoidsSilhouettePathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MedoidsSilhouettePath, k_MedoidsSilhouettePathNX);
}

TEST_CASE("ComplexCore::SilhouetteFilter: Means Test", "[ComplexCore][SilhouetteFilter]")
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    SilhouetteFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MeansClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MeansSilhouettePathNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MeansSilhouettePath, k_MeansSilhouettePathNX);
}

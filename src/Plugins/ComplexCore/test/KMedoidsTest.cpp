#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/KMedoidsFilter.hpp"

using namespace complex;

namespace
{
const std::string k_ClusterData = "ClusterData";
const std::string k_ClusterDataNX = k_ClusterData + "NX";

const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);
const DataPath k_ClusterDataPath = k_QuadGeomPath.createChildPath(k_ClusterData);
const DataPath k_ClusterDataPathNX = k_QuadGeomPath.createChildPath(k_ClusterDataNX);

const std::string k_ClusterIdsName = "ClusterIds";
const std::string k_MedoidsName = "ClusterMedoids";
const std::string k_ClusterIdsNameNX = k_ClusterIdsName + "NX";
const std::string k_MedoidsNameNX = k_MedoidsName + "NX";

const DataPath k_ClusterIdsPath = k_CellPath.createChildPath(k_ClusterIdsName);
const DataPath k_MedoidsPath = k_ClusterDataPath.createChildPath(k_MedoidsName);

const DataPath k_ClusterIdsPathNX = k_CellPath.createChildPath(k_ClusterIdsNameNX);
const DataPath k_MedoidsPathNX = k_ClusterDataPathNX.createChildPath(k_MedoidsNameNX);
} // namespace

TEST_CASE("ComplexCore::KMedoidsFilter: Valid Filter Execution", "[ComplexCore][KMedoidsFilter]")
{
  DataStructure dataStructure;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    KMedoidsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(KMedoidsFilter::k_InitClusters_Key, std::make_any<int32>(3));
    args.insertOrAssign(KMedoidsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(KMedoidsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(KMedoidsFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ClusterIdsNameNX));
    args.insertOrAssign(KMedoidsFilter::k_FeatureAMPath_Key, std::make_any<DataPath>(k_ClusterDataPathNX));
    args.insertOrAssign(KMedoidsFilter::k_MedoidsArrayName_Key, std::make_any<std::string>(k_MedoidsNameNX));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }
}

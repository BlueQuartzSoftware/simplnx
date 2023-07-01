#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/FindSlipTransmissionMetricsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
DataPath grainDataPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data});
DataPath neighborListPath = grainDataPath.createChildPath("NeighborList");
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "SurfaceMeshF1s";
const std::string k_f1spts = "SurfaceMeshF1spts";
const std::string k_f7s = "SurfaceMeshF7s";
const std::string k_mPrimes = "SurfaceMeshmPrimes";
} // namespace

TEST_CASE("OrientationAnalysis::FindSlipTransmissionMetricsFilter: Valid Filter Execution", "[OrientationAnalysis][FindSlipTransmissionMetricsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission.tar.gz",
                                                             "feature_boundary_neighbor_slip_transmission");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindSlipTransmissionMetricsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(neighborListPath));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_F1ListArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_F1sptListArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_F7ListArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(FindSlipTransmissionMetricsFilter::k_mPrimeListArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F1List"), grainDataPath.createChildPath(k_f1s));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F1sptList"), grainDataPath.createChildPath(k_f1spts));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("F7List"), grainDataPath.createChildPath(k_f7s));
  UnitTest::CompareNeighborLists<float32>(dataStructure, grainDataPath.createChildPath("mPrimeList"), grainDataPath.createChildPath(k_mPrimes));
}

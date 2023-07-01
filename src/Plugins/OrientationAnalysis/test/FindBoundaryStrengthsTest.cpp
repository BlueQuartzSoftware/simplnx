#include <catch2/catch.hpp>

#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/FindBoundaryStrengthsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
DataPath faceDataPath = DataPath({Constants::k_TriangleDataContainerName, Constants::k_FaceData});
DataPath faceLabelsPath = faceDataPath.createChildPath(Constants::k_FaceLabels);
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "SurfaceMeshF1s";
const std::string k_f1spts = "SurfaceMeshF1spts";
const std::string k_f7s = "SurfaceMeshF7s";
const std::string k_mPrimes = "SurfaceMeshmPrimes";
} // namespace

TEST_CASE("OrientationAnalysis::FindBoundaryStrengthsFilter: Valid Filter Execution", "[OrientationAnalysis][FindBoundaryStrengthsFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission.tar.gz",
                                                             "feature_boundary_neighbor_slip_transmission");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindBoundaryStrengthsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0, 0, 1}));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(FindBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1s"), faceDataPath.createChildPath(k_f1s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1spts"), faceDataPath.createChildPath(k_f1spts));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F7s"), faceDataPath.createChildPath(k_f7s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("mPrimes"), faceDataPath.createChildPath(k_mPrimes));
}

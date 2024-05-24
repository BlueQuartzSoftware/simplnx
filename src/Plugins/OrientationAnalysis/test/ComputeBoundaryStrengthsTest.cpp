#include <catch2/catch.hpp>

#include "simplnx/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/ComputeBoundaryStrengthsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
DataPath faceDataPath = DataPath({Constants::k_TriangleDataContainerName, Constants::k_FaceData});
DataPath faceLabelsPath = faceDataPath.createChildPath(Constants::k_FaceLabels);
DataPath avgQuatsPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_AvgQuats});
DataPath featurePhasesPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data, Constants::k_Phases});
DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures});

const std::string k_f1s = "F1List_COMPUTED";
const std::string k_f1spts = "F1sptList_COMPUTED";
const std::string k_f7s = "F7List_COMPUTED";
const std::string k_mPrimes = "mPrimeList_COMPUTED";
} // namespace

TEST_CASE("OrientationAnalysis::ComputeBoundaryStrengthsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeBoundaryStrengthsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "feature_boundary_neighbor_slip_transmission_1.tar.gz",
                                                              "feature_boundary_neighbor_slip_transmission");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/feature_boundary_neighbor_slip_transmission_1/6_6_feature_boundary_neighbor_slip_transmission.dream3d", unit_test::k_TestFilesDir)));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeBoundaryStrengthsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_Loading_Key, std::make_any<VectorFloat64Parameter::ValueType>(std::vector<float64>{0, 0, 1}));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sArrayName_Key, std::make_any<std::string>(k_f1s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF1sptsArrayName_Key, std::make_any<std::string>(k_f1spts));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshF7sArrayName_Key, std::make_any<std::string>(k_f7s));
    args.insertOrAssign(ComputeBoundaryStrengthsFilter::k_SurfaceMeshmPrimesArrayName_Key, std::make_any<std::string>(k_mPrimes));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/7_0_ComputeBoundaryStrengthsFilter.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1s"), faceDataPath.createChildPath(k_f1s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F1spts"), faceDataPath.createChildPath(k_f1spts));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("F7s"), faceDataPath.createChildPath(k_f7s));
  UnitTest::CompareArrays<float32>(dataStructure, faceDataPath.createChildPath("mPrimes"), faceDataPath.createChildPath(k_mPrimes));
}

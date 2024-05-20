#include "OrientationAnalysis/Filters/ComputeFaceIPFColoringFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
inline constexpr StringLiteral k_FaceIPFColors("SurfaceMeshFaceIPFColors");
inline constexpr StringLiteral k_NXFaceIPFColors("NXFaceIPFColors");

DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
DataPath featureDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_Grain_Data);
DataPath avgEulerAnglesPath = featureDataPath.createChildPath(nx::core::Constants::k_AvgEulerAngles);
DataPath featurePhasesPath = featureDataPath.createChildPath(nx::core::Constants::k_Phases);
DataPath crystalStructurePath = smallIn100Group.createChildPath(nx::core::Constants::k_Phase_Data).createChildPath(nx::core::Constants::k_CrystalStructures);
DataPath avgQuatsPath = featureDataPath.createChildPath("AvgQuats");

DataPath triangleDataContainerPath({nx::core::Constants::k_TriangleDataContainerName});
DataPath faceDataGroup = triangleDataContainerPath.createChildPath(nx::core::Constants::k_FaceData);

DataPath faceLabels = faceDataGroup.createChildPath(nx::core::Constants::k_FaceLabels);
DataPath faceNormals = faceDataGroup.createChildPath(nx::core::Constants::k_FaceNormals);
DataPath faceAreas = faceDataGroup.createChildPath(nx::core::Constants::k_FaceAreas);
} // namespace

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Valid filter execution", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFaceIPFColoringFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
  args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceIPFColors));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // compare the resulting face IPF Colors array
  DataPath exemplarPath = faceDataGroup.createChildPath(::k_FaceIPFColors);
  DataPath generatedPath = faceDataGroup.createChildPath(::k_NXFaceIPFColors);
  CompareArrays<uint8>(dataStructure, exemplarPath, generatedPath);
}

TEST_CASE("OrientationAnalysis::ComputeFaceIPFColoringFilter: Invalid filter execution", "[OrientationAnalysis][ComputeFaceIPFColoringFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFaceIPFColoringFilter filter;
  Arguments args;

  SECTION("Inconsistent face data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceIPFColors));
  }
  SECTION("Inconsistent cell data tuple dimensions")
  {
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(ComputeFaceIPFColoringFilter::k_SurfaceMeshFaceIPFColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceIPFColors));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
}

#include "OrientationAnalysis/Filters/ConvertOrientations.hpp"
#include "OrientationAnalysis/Filters/GenerateFaceMisorientationColoringFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::UnitTest;

namespace
{
inline constexpr StringLiteral k_FaceMisorientationColors("SurfaceMeshFaceMisorientationColors");
inline constexpr StringLiteral k_NXFaceMisorientationColors("NXFaceMisorientationColors");
inline constexpr StringLiteral k_AvgQuats("AvgQuats");

DataPath smallIn100Group({complex::Constants::k_SmallIN100});
DataPath featureDataPath = smallIn100Group.createChildPath(complex::Constants::k_Grain_Data);
DataPath avgEulerAnglesPath = featureDataPath.createChildPath(complex::Constants::k_AvgEulerAngles);
DataPath featurePhasesPath = featureDataPath.createChildPath(complex::Constants::k_Phases);
DataPath crystalStructurePath = smallIn100Group.createChildPath(complex::Constants::k_Phase_Data).createChildPath(complex::Constants::k_CrystalStructures);
DataPath avgQuatsPath = featureDataPath.createChildPath(k_AvgQuats);

DataPath triangleDataContainerPath({complex::Constants::k_TriangleDataContainerName});
DataPath faceDataGroup = triangleDataContainerPath.createChildPath(complex::Constants::k_FaceData);

DataPath faceLabels = faceDataGroup.createChildPath(complex::Constants::k_FaceLabels);
DataPath faceNormals = faceDataGroup.createChildPath(complex::Constants::k_FaceNormals);
DataPath faceAreas = faceDataGroup.createChildPath(complex::Constants::k_FaceAreas);
} // namespace

TEST_CASE("OrientationAnalysis::GenerateFaceMisorientationColoringFilter: Valid filter execution", "[OrientationAnalysis][GenerateFaceMisorientationColoringFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Convert the AvgEulerAngles array to AvgQuats for use in GenerateFaceMisorientationColoringFilter input
  {
    // Instantiate the filter, and an Arguments Object
    ConvertOrientations filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<uint64>(0));
    args.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<uint64>(2));
    args.insertOrAssign(ConvertOrientations::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
    args.insertOrAssign(ConvertOrientations::k_OutputOrientationArrayName_Key, std::make_any<std::string>(k_AvgQuats));

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // GenerateFaceMisorientationColoringFilter
  {
    // Instantiate the filter, and an Arguments Object
    GenerateFaceMisorientationColoringFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceMisorientationColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // compare the resulting face IPF Colors array
  DataPath exemplarPath = faceDataGroup.createChildPath(::k_FaceMisorientationColors);
  DataPath generatedPath = faceDataGroup.createChildPath(::k_NXFaceMisorientationColors);
  CompareArrays<float32>(dataStructure, exemplarPath, generatedPath);
}

TEST_CASE("OrientationAnalysis::GenerateFaceMisorientationColoringFilter: Invalid filter execution", "[OrientationAnalysis][GenerateFaceMisorientationColoringFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  GenerateFaceMisorientationColoringFilter filter;
  Arguments args;

  SECTION("Inconsistent cell data tuple dimensions")
  {
    // Convert the AvgEulerAngles array to AvgQuats for use in GenerateFaceMisorientationColoringFilter input
    {
      // Instantiate the filter, and an Arguments Object
      ConvertOrientations convertOrientationsFilter;
      Arguments convertOrientationsArgs;

      // Create default Parameters for the filter.
      convertOrientationsArgs.insertOrAssign(ConvertOrientations::k_InputType_Key, std::make_any<uint64>(0));
      convertOrientationsArgs.insertOrAssign(ConvertOrientations::k_OutputType_Key, std::make_any<uint64>(2));
      convertOrientationsArgs.insertOrAssign(ConvertOrientations::k_InputOrientationArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
      convertOrientationsArgs.insertOrAssign(ConvertOrientations::k_OutputOrientationArrayName_Key, std::make_any<std::string>(k_AvgQuats));

      // Execute the filter and check the result
      auto convertOrientationsResult = convertOrientationsFilter.execute(dataStructure, convertOrientationsArgs);
      COMPLEX_RESULT_REQUIRE_VALID(convertOrientationsResult.result);
    }

    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceMisorientationColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));
  }

  SECTION("Missing input data path")
  {
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsPath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructurePath));
    args.insertOrAssign(GenerateFaceMisorientationColoringFilter::k_SurfaceMeshFaceMisorientationColorsArrayName_Key, std::make_any<std::string>(::k_NXFaceMisorientationColors));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}

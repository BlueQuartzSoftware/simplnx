#include "OrientationAnalysis/Filters/FindGBCDFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;

namespace
{
inline constexpr StringLiteral k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

} // namespace

TEST_CASE("OrientationAnalysis::FindGBCD", "[OrientationAnalysis][FindGBCD]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  DataPath featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);

  DataPath ensembleDataPath = smallIn100Group.createChildPath(Constants::k_Phase_Data);
  DataPath crystalStructurePath = ensembleDataPath.createChildPath(Constants::k_CrystalStructures);

  DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindGBCDFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindGBCDFilter::k_GBCDRes_Key, std::make_any<Float32Parameter::ValueType>(9.0F));

    args.insertOrAssign(FindGBCDFilter::k_TriangleGeometry_Key, std::make_any<GeometrySelectionParameter::ValueType>(triangleDataContainerPath));

    args.insertOrAssign(FindGBCDFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceLabels));
    args.insertOrAssign(FindGBCDFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceNormals));
    args.insertOrAssign(FindGBCDFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(faceAreas));
    args.insertOrAssign(FindGBCDFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(avgEulerAnglesPath));
    args.insertOrAssign(FindGBCDFilter::k_FeaturePhasesArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(featurePhasesPath));
    args.insertOrAssign(FindGBCDFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(FindGBCDFilter::k_FaceEnsembleAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_FaceEnsembleDataPath));
    args.insertOrAssign(FindGBCDFilter::k_GBCDArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(Constants::k_GBCD_Name));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output GBCD Data
  {
    const DataPath k_GeneratedDataPath = faceEnsemblePath.createChildPath(Constants::k_GBCD_Name);
    const DataPath k_ExemplarArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(Constants::k_GBCD_Name);

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_gbcd.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}

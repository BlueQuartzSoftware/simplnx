#include "SimplnxCore/Filters/FeatureFaceCurvatureFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
void CompareDataArrays(const DataStructure& dataStructure, const DataPath& arrayPath1, const DataPath& arrayPath2)
{
  const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(arrayPath1);
  const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(arrayPath2);

  REQUIRE(dataArray1 != nullptr);
  REQUIRE(dataArray2 != nullptr);

  const auto length = dataArray1->getSize();
  REQUIRE(dataArray1->getSize() == dataArray2->getSize());

  const Float64AbstractDataStore& dataStore1 = dataArray1->getDataStoreRef();
  const Float64AbstractDataStore& dataStore2 = dataArray2->getDataStoreRef();

  for(usize i = 0; i < length; i++)
  {
    REQUIRE(UnitTest::CloseEnough<float64>(dataStore1[i], dataStore2[i]));
  }
}
} // namespace

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: Test Algorithm", "[FeatureFaceCurvatureFilter]")
{
  DataPath triangleGeomPath({"TriangleDataContainer"});
  DataPath faceAttribMatrixPath = triangleGeomPath.createChildPath("FaceData");

  DataPath k_PrincipalCurvature1_Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D-2");
  DataPath k_PrincipalCurvature2_Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D-2");
  DataPath k_PrincipalDirection1_Path = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D-2");
  DataPath k_PrincipalDirection2_Path = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D-2");
  DataPath k_GaussianCurvature_Path = faceAttribMatrixPath.createChildPath("GaussianCurvatures-2");
  DataPath k_MeanCurvature_Path = faceAttribMatrixPath.createChildPath("MeanCurvatures-2");
  DataPath k_WeingartenMatrix_Path = faceAttribMatrixPath.createChildPath("WeingartenMatrix-2");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_5_Goldfeather.tar.gz", "6_5_Goldfeather");

  FeatureFaceCurvatureFilter filter;

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_Goldfeather/6_5_Goldfeather.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  Arguments args;
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_NeighborhoodRing_Key, std::make_any<int32>(3));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputePrincipalDirection_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeGaussianCurvature_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeMeanCurvaturePath_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeWeingartenMatrix_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_UseFaceNormals_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceAttribMatrixPath_Key, std::make_any<DataPath>(faceAttribMatrixPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceLabelsPath_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceLabels")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FeatureFaceIdsPath_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FeatureFaceId")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceNormalsPath_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceNormals")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceCentroidsPath_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceCentroids")));

  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature1Path_Key, std::make_any<DataPath>(k_PrincipalCurvature1_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature2Path_Key, std::make_any<DataPath>(k_PrincipalCurvature2_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection1Path_Key, std::make_any<DataPath>(k_PrincipalDirection1_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection2Path_Key, std::make_any<DataPath>(k_PrincipalDirection2_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_GaussianCurvaturePath_Key, std::make_any<DataPath>(k_GaussianCurvature_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_MeanCurvaturePath_Key, std::make_any<DataPath>(k_MeanCurvature_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_WeingartenMatrixPath_Key, std::make_any<DataPath>(k_WeingartenMatrix_Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare arrays
  {
    INFO("Principal Curvature 1");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D");
    ::CompareDataArrays(dataStructure, path1, k_PrincipalCurvature1_Path);
  }

  {
    INFO("Principal Curvature 2");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D");
    ::CompareDataArrays(dataStructure, path1, k_PrincipalCurvature2_Path);
  }

  {
    INFO("Principal Direction 1");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D");
    ::CompareDataArrays(dataStructure, path1, k_PrincipalDirection1_Path);
  }

  {
    INFO("Principal Direction 2");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D");
    ::CompareDataArrays(dataStructure, path1, k_PrincipalDirection2_Path);
  }

  {
    INFO("Gaussian Curvatures");
    DataPath path1 = faceAttribMatrixPath.createChildPath("GaussianCurvatures");
    ::CompareDataArrays(dataStructure, path1, k_GaussianCurvature_Path);
  }

  {
    INFO("Mean Curvatures");
    DataPath path1 = faceAttribMatrixPath.createChildPath("MeanCurvatures");
    ::CompareDataArrays(dataStructure, path1, k_MeanCurvature_Path);
  }
}

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FeatureFaceCurvatureFilter.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <catch2/catch.hpp>

using namespace complex;

inline void CompareDataArrays(const DataStructure& dataStructure, const DataPath& arrayPath1, const DataPath& arrayPath2)
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

TEST_CASE("ComplexCore::FeatureFaceCurvatureFilter: Test Algorithm", "[FeatureFaceCurvatureFilter]")
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

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_5_Goldfeather.tar.gz", "6_5_Goldfeather");

  FeatureFaceCurvatureFilter filter;

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_Goldfeather/6_5_Goldfeather.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  Arguments args;
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_TriangleGeom_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_NeighborhoodRing_Key, std::make_any<int32>(3));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputePrincipalDirection_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeGaussianCurvature_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeMeanCurvature_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeWeingartenMatrix_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_UseFaceNormals_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceAttribMatrix_Key, std::make_any<DataPath>(faceAttribMatrixPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceLabels_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceLabels")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FeatureFaceIds_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FeatureFaceId")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceNormals_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceNormals")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceCentroids_Key, std::make_any<DataPath>(faceAttribMatrixPath.createChildPath("FaceCentroids")));

  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature1_Key, std::make_any<DataPath>(k_PrincipalCurvature1_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature2_Key, std::make_any<DataPath>(k_PrincipalCurvature2_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection1_Key, std::make_any<DataPath>(k_PrincipalDirection1_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection2_Key, std::make_any<DataPath>(k_PrincipalDirection2_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_GaussianCurvature_Key, std::make_any<DataPath>(k_GaussianCurvature_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_MeanCurvature_Key, std::make_any<DataPath>(k_MeanCurvature_Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_WeingartenMatrix_Key, std::make_any<DataPath>(k_WeingartenMatrix_Path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare arrays
  SECTION("Principal Curvature 1")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D");
    DataPath path2 = k_PrincipalCurvature1_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);

    CompareDataArrays(dataStructure, path1, path2);
  }

  SECTION("Principal Curvature 2")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D");
    DataPath path2 = k_PrincipalCurvature2_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays(dataStructure, path1, path2);
  }

  SECTION("Principal Direction 1")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D");
    DataPath path2 = k_PrincipalDirection1_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays(dataStructure, path1, path2);
  }

  SECTION("Principal Direction 2")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D");
    DataPath path2 = k_PrincipalDirection2_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays(dataStructure, path1, path2);
  }

  SECTION("Gaussian Curvatures")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("GaussianCurvatures");
    DataPath path2 = k_GaussianCurvature_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays(dataStructure, path1, path2);
  }

  SECTION("Mean Curvatures")
  {
    DataPath path1 = faceAttribMatrixPath.createChildPath("MeanCurvatures");
    DataPath path2 = k_MeanCurvature_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays(dataStructure, path1, path2);
  }
}

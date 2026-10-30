#include "SimplnxCore/Filters/FeatureFaceCurvatureFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>

using namespace nx::core;

namespace
{
enum class FloatComparison
{
  Standard,
  Absolute
};

template <FloatComparison ComparisonValue = FloatComparison::Standard>
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
    INFO(fmt::format("i = {} | d1 = {} | d2 = {}", i, dataStore1[i], dataStore2[i]));
    if constexpr(ComparisonValue == FloatComparison::Standard)
    {
      REQUIRE(UnitTest::CloseEnough<float64>(dataStore1[i], dataStore2[i]));
    }
    else
    {
      REQUIRE(UnitTest::CloseEnoughAbs<float64>(dataStore1[i], dataStore2[i]));
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: Test Algorithm", "[FeatureFaceCurvatureFilter]")
{
  UnitTest::LoadPlugins();

  DataPath triangleGeomPath({"TriangleDataContainer"});
  DataPath faceAttribMatrixPath = triangleGeomPath.createChildPath("FaceData");

  DataPath k_PrincipalCurvature1_Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D-2");
  DataPath k_PrincipalCurvature2_Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D-2");
  DataPath k_PrincipalDirection1_Path = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D-2");
  DataPath k_PrincipalDirection2_Path = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D-2");
  DataPath k_GaussianCurvature_Path = faceAttribMatrixPath.createChildPath("GaussianCurvatures-2");
  DataPath k_MeanCurvature_Path = faceAttribMatrixPath.createChildPath("MeanCurvatures-2");
  DataPath k_WeingartenMatrix_Path = faceAttribMatrixPath.createChildPath("WeingartenMatrix-2");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_Goldfeather.tar.gz", "6_5_Goldfeather");

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

  /*
   * The principal directions can be sign flipped. This is due to Eigen::SelfAdjointEigenSolver
   * which returns unique eigenvectors up to a sign.
   */
  {
    INFO("Principal Direction 1");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D");
    DataPath path2 = k_PrincipalDirection1_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays<FloatComparison::Absolute>(dataStructure, path1, path2);
  }

  {
    INFO("Principal Direction 2");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D");
    DataPath path2 = k_PrincipalDirection2_Path;
    const auto* dataArray1 = dataStructure.getDataAs<Float64Array>(path1);
    const auto* dataArray2 = dataStructure.getDataAs<Float64Array>(path2);
    CompareDataArrays<FloatComparison::Absolute>(dataStructure, path1, path2);
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: NaN patch data", "[FeatureFaceCurvatureFilter][NaNPatch]")
{
  UnitTest::LoadPlugins();
  const bool useNormals = GENERATE(false, true);
  const bool optionalOutputs = GENERATE(false, true);
  const usize badComponent = GENERATE(0, 1, 2, 3, 4, 5);
  // Nine faces leave eight valid samples for the seven-coefficient cubic fit.
  const usize badFace = GENERATE(8, 0, 4);
  const usize validCount = GENERATE(8, 1, 0);
  CAPTURE(useNormals, optionalOutputs, badComponent, badFace, validCount);

  DataStructure dataStructure;
  auto* geom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geom != nullptr);
  auto* faceData = AttributeMatrix::Create(dataStructure, "FaceData", {9}, geom->getId());
  REQUIRE(faceData != nullptr);
  geom->setFaceAttributeMatrix(*faceData);
  auto* vertices = Float32Array::Create(dataStructure, "Vertices", std::make_unique<DataStore<float32>>(ShapeType{19}, ShapeType{3}, 0.0F), geom->getId());
  auto* faces = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", std::make_unique<DataStore<IGeometry::MeshIndexType>>(ShapeType{9}, ShapeType{3}, 0), geom->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  geom->setVertices(*vertices);
  geom->setFaceList(*faces);
  auto* labels = Int32Array::Create(dataStructure, "Labels", std::make_unique<DataStore<int32>>(ShapeType{9}, ShapeType{2}, 1), faceData->getId());
  auto* ids = Int32Array::Create(dataStructure, "Ids", std::make_unique<DataStore<int32>>(ShapeType{9}, ShapeType{1}, 1), faceData->getId());
  auto* normals = Float64Array::Create(dataStructure, "Normals", std::make_unique<DataStore<float64>>(ShapeType{9}, ShapeType{3}, 0.0), faceData->getId());
  auto* centroids = Float64Array::Create(dataStructure, "Centroids", std::make_unique<DataStore<float64>>(ShapeType{9}, ShapeType{3}, 0.0), faceData->getId());
  REQUIRE(labels != nullptr);
  REQUIRE(ids != nullptr);
  REQUIRE(normals != nullptr);
  REQUIRE(centroids != nullptr);

  for(usize t = 0; t < 9; ++t)
  {
    // All faces share vertex 0, so every one-ring patch is the sorted set {0,...,8}.
    // The other vertices are 1.5*c +/- (-cy,cx), giving centroid c and normal +Z.
    const float64 x = 1.0 + static_cast<float64>(t % 3);
    const float64 y = 1.0 + static_cast<float64>((t / 3) * (t / 3));
    vertices->setComponent(2 * t + 1, 0, static_cast<float32>(1.5 * x + y));
    vertices->setComponent(2 * t + 1, 1, static_cast<float32>(1.5 * y - x));
    vertices->setComponent(2 * t + 2, 0, static_cast<float32>(1.5 * x - y));
    vertices->setComponent(2 * t + 2, 1, static_cast<float32>(1.5 * y + x));
    faces->setComponent(t, 1, 2 * t + 1);
    faces->setComponent(t, 2, 2 * t + 2);
    labels->setComponent(t, 1, 2);
    normals->setComponent(t, 2, 1.0);
    centroids->setComponent(t, 0, x);
    centroids->setComponent(t, 1, y);
    const bool invalid = validCount == 0 || (validCount == 1 ? t != badFace : t == badFace);
    if(invalid)
    {
      auto* data = badComponent < 3 ? normals : centroids;
      data->setComponent(t, badComponent % 3, std::numeric_limits<float64>::quiet_NaN());
    }
  }

  const DataPath geomPath({"Triangles"});
  const DataPath facePath = geomPath.createChildPath("FaceData");
  Arguments args;
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceAttribMatrixPath_Key, std::make_any<DataPath>(facePath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_NeighborhoodRing_Key, std::make_any<int32>(1));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_UseFaceNormals_Key, std::make_any<bool>(useNormals));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputePrincipalDirection_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeGaussianCurvature_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeMeanCurvaturePath_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeWeingartenMatrix_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceLabelsPath_Key, std::make_any<DataPath>(facePath.createChildPath("Labels")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FeatureFaceIdsPath_Key, std::make_any<DataPath>(facePath.createChildPath("Ids")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceNormalsPath_Key, std::make_any<DataPath>(facePath.createChildPath("Normals")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceCentroidsPath_Key, std::make_any<DataPath>(facePath.createChildPath("Centroids")));
  const std::array outputKeys = {FeatureFaceCurvatureFilter::k_PrincipalCurvature1Path_Key, FeatureFaceCurvatureFilter::k_PrincipalCurvature2Path_Key,
                                 FeatureFaceCurvatureFilter::k_GaussianCurvaturePath_Key,   FeatureFaceCurvatureFilter::k_MeanCurvaturePath_Key,
                                 FeatureFaceCurvatureFilter::k_WeingartenMatrixPath_Key,    FeatureFaceCurvatureFilter::k_PrincipalDirection1Path_Key,
                                 FeatureFaceCurvatureFilter::k_PrincipalDirection2Path_Key};
  for(usize i = 0; i < outputKeys.size(); ++i)
  {
    args.insertOrAssign(outputKeys[i], std::make_any<DataPath>(facePath.createChildPath("Output" + std::to_string(i))));
  }
  FeatureFaceCurvatureFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(usize i = 0; i < (optionalOutputs ? outputKeys.size() : 2); ++i)
  {
    const auto* output = dataStructure.getDataAs<Float64Array>(facePath.createChildPath("Output" + std::to_string(i)));
    REQUIRE(output != nullptr);
    for(usize t = 0; t < 9; ++t)
    {
      for(usize c = 0; c < output->getNumberOfComponents(); ++c)
      {
        CAPTURE(i, t, c);
        const float64 value = output->getDataStoreRef().getComponentValue(t, c);
        if(validCount < 2 || t == badFace)
        {
          CHECK(std::isnan(value));
        }
        else if(i < 5)
        {
          // Closed-form oracle: z=0 has zero Hessian, principal/mean/Gaussian
          // curvatures and Weingarten matrix, including after removing a sample.
          CHECK(value == Approx(0.0).margin(1.0e-12));
        }
        else
        {
          CHECK(std::isfinite(value));
        }
      }
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

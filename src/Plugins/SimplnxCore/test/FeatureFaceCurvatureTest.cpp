#include "SimplnxCore/Filters/Algorithms/FeatureFaceCurvature.hpp"
#include "SimplnxCore/Filters/FeatureFaceCurvatureFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

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
  const auto* dataArray1Ptr = dataStructure.getDataAs<Float64ArrayType>(arrayPath1);
  const auto* dataArray2Ptr = dataStructure.getDataAs<Float64ArrayType>(arrayPath2);

  REQUIRE(dataArray1Ptr != nullptr);
  REQUIRE(dataArray2Ptr != nullptr);

  const auto length = dataArray1Ptr->getSize();
  REQUIRE(dataArray1Ptr->getSize() == dataArray2Ptr->getSize());

  const Float64AbstractDataStoreType& dataStore1 = dataArray1Ptr->getDataStoreRef();
  const Float64AbstractDataStoreType& dataStore2 = dataArray2Ptr->getDataStoreRef();

  for(usize i = 0; i < length; i++)
  {
    INFO(fmt::format("i = {} | d1 = {} | d2 = {}", i, dataStore1[i], dataStore2[i]));
    if constexpr(ComparisonValue == FloatComparison::Standard)
    {
      REQUIRE(unit_test::CloseEnough<float64>(dataStore1[i], dataStore2[i]));
    }
    else
    {
      REQUIRE(unit_test::CloseEnoughAbs<float64>(dataStore1[i], dataStore2[i]));
    }
  }
}
} // namespace

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: Test Algorithm", "[FeatureFaceCurvatureFilter]")
{
  unit_test::LoadPlugins();

  DataPath triangleGeomPath({"TriangleDataContainer"});
  DataPath faceAttribMatrixPath = triangleGeomPath.createChildPath("FaceData");

  DataPath kPrincipalCurvature1Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D-2");
  DataPath kPrincipalCurvature2Path = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D-2");
  DataPath kPrincipalDirection1Path = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D-2");
  DataPath kPrincipalDirection2Path = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D-2");
  DataPath kGaussianCurvaturePath = faceAttribMatrixPath.createChildPath("GaussianCurvatures-2");
  DataPath kMeanCurvaturePath = faceAttribMatrixPath.createChildPath("MeanCurvatures-2");
  DataPath kWeingartenMatrixPath = faceAttribMatrixPath.createChildPath("WeingartenMatrix-2");

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_5_Goldfeather.tar.gz", "6_5_Goldfeather");

  FeatureFaceCurvatureFilter filter;

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_5_Goldfeather/6_5_Goldfeather.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(exemplarFilePath);

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

  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature1Path_Key, std::make_any<DataPath>(kPrincipalCurvature1Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalCurvature2Path_Key, std::make_any<DataPath>(kPrincipalCurvature2Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection1Path_Key, std::make_any<DataPath>(kPrincipalDirection1Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_PrincipalDirection2Path_Key, std::make_any<DataPath>(kPrincipalDirection2Path));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_GaussianCurvaturePath_Key, std::make_any<DataPath>(kGaussianCurvaturePath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_MeanCurvaturePath_Key, std::make_any<DataPath>(kMeanCurvaturePath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_WeingartenMatrixPath_Key, std::make_any<DataPath>(kWeingartenMatrixPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare arrays
  {
    INFO("Principal Curvature 1");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature1_D3D");
    ::CompareDataArrays(dataStructure, path1, kPrincipalCurvature1Path);
  }

  {
    INFO("Principal Curvature 2");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalCurvature2_D3D");
    ::CompareDataArrays(dataStructure, path1, kPrincipalCurvature2Path);
  }

  /*
   * The principal directions can be sign flipped. This is due to Eigen::SelfAdjointEigenSolver
   * which returns unique eigenvectors up to a sign.
   */
  {
    INFO("Principal Direction 1");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection1_D3D");
    const DataPath& path2 = kPrincipalDirection1Path;
    const auto* dataArray1Ptr = dataStructure.getDataAs<Float64ArrayType>(path1);
    const auto* dataArray2Ptr = dataStructure.getDataAs<Float64ArrayType>(path2);
    CompareDataArrays<FloatComparison::Absolute>(dataStructure, path1, path2);
  }

  {
    INFO("Principal Direction 2");
    DataPath path1 = faceAttribMatrixPath.createChildPath("PrincipalDirection2_D3D");
    const DataPath& path2 = kPrincipalDirection2Path;
    const auto* dataArray1Ptr = dataStructure.getDataAs<Float64ArrayType>(path1);
    const auto* dataArray2Ptr = dataStructure.getDataAs<Float64ArrayType>(path2);
    CompareDataArrays<FloatComparison::Absolute>(dataStructure, path1, path2);
  }

  {
    INFO("Gaussian Curvatures");
    DataPath path1 = faceAttribMatrixPath.createChildPath("GaussianCurvatures");
    ::CompareDataArrays(dataStructure, path1, kGaussianCurvaturePath);
  }

  {
    INFO("Mean Curvatures");
    DataPath path1 = faceAttribMatrixPath.createChildPath("MeanCurvatures");
    ::CompareDataArrays(dataStructure, path1, kMeanCurvaturePath);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: NaN patch data", "[FeatureFaceCurvatureFilter][NaNPatch]")
{
  unit_test::LoadPlugins();
  const bool useNormals = GENERATE(false, true);
  const bool optionalOutputs = GENERATE(false, true);
  const usize badComponent = GENERATE(0, 1, 2, 3, 4, 5);
  // Nine faces leave eight valid samples for the seven-coefficient cubic fit.
  const usize badFace = GENERATE(8, 0, 4);
  const usize validCount = GENERATE(8, 1, 0);
  CAPTURE(useNormals, optionalOutputs, badComponent, badFace, validCount);

  DataStructure dataStructure;
  auto* geomPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geomPtr != nullptr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {9}, geomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  geomPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto* verticesPtr = Float32ArrayType::Create(dataStructure, "Vertices", std::make_unique<DataStore<float32>>(ShapeType{19}, ShapeType{3}, 0.0F), geomPtr->getId());
  auto* facesPtr = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", std::make_unique<DataStore<IGeometry::MeshIndexType>>(ShapeType{9}, ShapeType{3}, 0), geomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  geomPtr->setVertices(*verticesPtr);
  geomPtr->setFaceList(*facesPtr);
  auto* labelsPtr = Int32ArrayType::Create(dataStructure, "Labels", std::make_unique<DataStore<int32>>(ShapeType{9}, ShapeType{2}, 1), faceDataPtr->getId());
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", std::make_unique<DataStore<int32>>(ShapeType{9}, ShapeType{1}, 1), faceDataPtr->getId());
  auto* normalsPtr = Float64ArrayType::Create(dataStructure, "Normals", std::make_unique<DataStore<float64>>(ShapeType{9}, ShapeType{3}, 0.0), faceDataPtr->getId());
  auto* centroidsPtr = Float64ArrayType::Create(dataStructure, "Centroids", std::make_unique<DataStore<float64>>(ShapeType{9}, ShapeType{3}, 0.0), faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(normalsPtr != nullptr);
  REQUIRE(centroidsPtr != nullptr);

  for(usize t = 0; t < 9; ++t)
  {
    // All faces share vertex 0, so every one-ring patch is the sorted set {0,...,8}.
    // The other vertices are 1.5*c +/- (-cy,cx), giving centroid c and normal +Z.
    const float64 x = 1.0 + static_cast<float64>(t % 3);
    const float64 y = 1.0 + static_cast<float64>((t / 3) * (t / 3));
    verticesPtr->setComponent(2 * t + 1, 0, static_cast<float32>(1.5 * x + y));
    verticesPtr->setComponent(2 * t + 1, 1, static_cast<float32>(1.5 * y - x));
    verticesPtr->setComponent(2 * t + 2, 0, static_cast<float32>(1.5 * x - y));
    verticesPtr->setComponent(2 * t + 2, 1, static_cast<float32>(1.5 * y + x));
    facesPtr->setComponent(t, 1, 2 * t + 1);
    facesPtr->setComponent(t, 2, 2 * t + 2);
    labelsPtr->setComponent(t, 1, 2);
    normalsPtr->setComponent(t, 2, 1.0);
    centroidsPtr->setComponent(t, 0, x);
    centroidsPtr->setComponent(t, 1, y);
    const bool invalid = validCount == 0 || (validCount == 1 ? t != badFace : t == badFace);
    if(invalid)
    {
      auto* dataPtr = badComponent < 3 ? normalsPtr : centroidsPtr;
      dataPtr->setComponent(t, badComponent % 3, std::numeric_limits<float64>::quiet_NaN());
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(usize i = 0; i < (optionalOutputs ? outputKeys.size() : 2); ++i)
  {
    const auto* outputPtr = dataStructure.getDataAs<Float64ArrayType>(facePath.createChildPath("Output" + std::to_string(i)));
    REQUIRE(outputPtr != nullptr);
    for(usize t = 0; t < 9; ++t)
    {
      for(usize c = 0; c < outputPtr->getNumberOfComponents(); ++c)
      {
        CAPTURE(i, t, c);
        const float64 value = outputPtr->getDataStoreRef().getComponentValue(t, c);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: Single triangle face", "[SimplnxCore][FeatureFaceCurvatureFilter][SingleTriangle]")
{
  const bool useNormals = GENERATE(false, true);
  const bool optionalOutputs = GENERATE(false, true);
  CAPTURE(useNormals, optionalOutputs);

  DataStructure dataStructure;
  auto* geomPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geomPtr != nullptr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {1}, geomPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  geomPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto* verticesPtr = Float32ArrayType::Create(dataStructure, "Vertices", std::make_unique<DataStore<float32>>(ShapeType{3}, ShapeType{3}, 0.0F), geomPtr->getId());
  auto* facesPtr = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", std::make_unique<DataStore<IGeometry::MeshIndexType>>(ShapeType{1}, ShapeType{3}, 0), geomPtr->getId());
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  geomPtr->setVertices(*verticesPtr);
  geomPtr->setFaceList(*facesPtr);
  auto* labelsPtr = Int32ArrayType::Create(dataStructure, "Labels", std::make_unique<DataStore<int32>>(ShapeType{1}, ShapeType{2}, 1), faceDataPtr->getId());
  auto* idsPtr = Int32ArrayType::Create(dataStructure, "Ids", std::make_unique<DataStore<int32>>(ShapeType{1}, ShapeType{1}, 1), faceDataPtr->getId());
  auto* normalsPtr = Float64ArrayType::Create(dataStructure, "Normals", std::make_unique<DataStore<float64>>(ShapeType{1}, ShapeType{3}, 0.0), faceDataPtr->getId());
  auto* centroidsPtr = Float64ArrayType::Create(dataStructure, "Centroids", std::make_unique<DataStore<float64>>(ShapeType{1}, ShapeType{3}, 0.0), faceDataPtr->getId());
  REQUIRE(labelsPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(normalsPtr != nullptr);
  REQUIRE(centroidsPtr != nullptr);

  // Triangle (0,0,0), (1,0,0), (0,1,0): centroid (1/3,1/3,0), normal +Z.
  verticesPtr->setComponent(1, 0, 1.0F);
  verticesPtr->setComponent(2, 1, 1.0F);
  facesPtr->setComponent(0, 1, 1);
  facesPtr->setComponent(0, 2, 2);
  labelsPtr->setComponent(0, 1, 2);
  normalsPtr->setComponent(0, 2, 1.0);
  centroidsPtr->setComponent(0, 0, 1.0 / 3.0);
  centroidsPtr->setComponent(0, 1, 1.0 / 3.0);

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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Independent oracle: one centroid and normal cannot determine surface curvature.
  // The insufficient-patch convention requires NaN in every requested component.
  for(usize i = 0; i < (optionalOutputs ? outputKeys.size() : 2); ++i)
  {
    const auto* outputPtr = dataStructure.getDataAs<Float64ArrayType>(facePath.createChildPath("Output" + std::to_string(i)));
    REQUIRE(outputPtr != nullptr);
    REQUIRE(outputPtr->getNumberOfTuples() == 1);
    for(usize c = 0; c < outputPtr->getNumberOfComponents(); ++c)
    {
      CAPTURE(i, c);
      CHECK(std::isnan(outputPtr->getDataStoreRef().getComponentValue(0, c)));
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: Patch solvability", "[SimplnxCore][FeatureFaceCurvatureFilter][PatchSolvability]")
{
  const bool useNormals = GENERATE(false, true);
  const bool optionalOutputs = GENERATE(false, true);
  const usize nonSeedCount = GENERATE(1, 2, 3, 6, 7, 8);
  const bool duplicateSeed = GENERATE(false, true);
  const usize faceCount = nonSeedCount + 1 + (duplicateSeed ? 1 : 0);
  CAPTURE(useNormals, optionalOutputs, nonSeedCount, duplicateSeed);

  DataStructure dataStructure;
  auto* geom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geom != nullptr);
  auto* faceData = AttributeMatrix::Create(dataStructure, "FaceData", {faceCount}, geom->getId());
  REQUIRE(faceData != nullptr);
  geom->setFaceAttributeMatrix(*faceData);
  auto* vertices = Float32ArrayType::Create(dataStructure, "Vertices", std::make_unique<DataStore<float32>>(ShapeType{2 * faceCount + 1}, ShapeType{3}, 0.0F), geom->getId());
  auto* faces = DataArray<IGeometry::MeshIndexType>::Create(dataStructure, "Faces", std::make_unique<DataStore<IGeometry::MeshIndexType>>(ShapeType{faceCount}, ShapeType{3}, 0), geom->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  geom->setVertices(*vertices);
  geom->setFaceList(*faces);
  auto* labels = Int32ArrayType::Create(dataStructure, "Labels", std::make_unique<DataStore<int32>>(ShapeType{faceCount}, ShapeType{2}, 1), faceData->getId());
  auto* ids = Int32ArrayType::Create(dataStructure, "Ids", std::make_unique<DataStore<int32>>(ShapeType{faceCount}, ShapeType{1}, 1), faceData->getId());
  auto* normals = Float64ArrayType::Create(dataStructure, "Normals", std::make_unique<DataStore<float64>>(ShapeType{faceCount}, ShapeType{3}, 0.0), faceData->getId());
  auto* centroids = Float64ArrayType::Create(dataStructure, "Centroids", std::make_unique<DataStore<float64>>(ShapeType{faceCount}, ShapeType{3}, 0.0), faceData->getId());
  REQUIRE(labels != nullptr);
  REQUIRE(ids != nullptr);
  REQUIRE(normals != nullptr);
  REQUIRE(centroids != nullptr);

  // All triangles share vertex 0, so the one-ring patch contains every face.
  // Sample z=-(x*x+y*y)/(2R). At seed 0 the normal is +Z and both
  // signed principal curvatures are +1/R. The quadratic is exact at this seed.
  constexpr float64 radius = 10.0;
  const std::array<std::array<float64, 2>, 9> samples = {{{0, 0}, {1, 0}, {0, 1}, {1, 1}, {-1, 1}, {2, 1}, {1, 2}, {-1, 2}, {2, -1}}};
  vertices->setComponent(0, 2, 1.0F);
  for(usize t = 0; t < faceCount; ++t)
  {
    const auto& sample = samples[t <= nonSeedCount ? t : 0];
    const float64 x = sample[0];
    const float64 y = sample[1];
    const float64 z = -(x * x + y * y) / (2.0 * radius);
    // Distinct vertices give each triangle the supplied centroid, including seed 0.
    vertices->setComponent(2 * t + 1, 0, static_cast<float32>(3.0 * x + 1.0));
    vertices->setComponent(2 * t + 1, 1, static_cast<float32>(3.0 * y + 1.0));
    vertices->setComponent(2 * t + 1, 2, static_cast<float32>(3.0 * z - 0.5));
    vertices->setComponent(2 * t + 2, 0, -1.0F);
    vertices->setComponent(2 * t + 2, 1, -1.0F);
    vertices->setComponent(2 * t + 2, 2, -0.5F);
    faces->setComponent(t, 1, 2 * t + 1);
    faces->setComponent(t, 2, 2 * t + 2);
    labels->setComponent(t, 1, 2);
    const float64 length = std::sqrt(1.0 + (x * x + y * y) / (radius * radius));
    normals->setComponent(t, 0, x / (radius * length));
    normals->setComponent(t, 1, y / (radius * length));
    normals->setComponent(t, 2, 1.0 / length);
    centroids->setComponent(t, 0, x);
    centroids->setComponent(t, 1, y);
    centroids->setComponent(t, 2, z);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  for(usize i = 0; i < (optionalOutputs ? outputKeys.size() : 2); ++i)
  {
    const auto* output = dataStructure.getDataAs<Float64ArrayType>(facePath.createChildPath("Output" + std::to_string(i)));
    REQUIRE(output != nullptr);
    // Only seed 0 has a horizontal tangent plane and the closed-form 1/R fit.
    for(usize c = 0; c < output->getNumberOfComponents(); ++c)
    {
      CAPTURE(i, c);
      const float64 value = output->getDataStoreRef().getComponentValue(0, c);
      if(nonSeedCount < (useNormals ? 7 : 3))
      {
        CHECK(std::isnan(value));
      }
      else if(i < 4)
      {
        const float64 expected = i == 2 ? 1.0 / (radius * radius) : 1.0 / radius;
        // Exact polynomial fit at the origin, with absolute roundoff tolerance.
        CHECK(value == Approx(expected).margin(1.0e-10));
      }
      else if(i == 4)
      {
        CHECK(value == Approx(c == 0 || c == 3 ? -1.0 / radius : 0.0).margin(1.0e-10));
      }
      else
      {
        CHECK(std::isfinite(value));
      }
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
constexpr int32 k_InjectedError = -56991;
const DataPath k_MeshGeomPath({"Triangles"});
const DataPath k_MeshFacePath = k_MeshGeomPath.createChildPath("FaceData");
using SupportListType = TriangleGeom::ElementDynamicListType;
using SupportIndexType = IGeometry::MeshIndexType;

// These stores inject ordinary Result failures only. They do not simulate OOC or allocation failure.
template <typename T>
class InjectedFailureStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;
  ~InjectedFailureStore() noexcept override = default;
  InjectedFailureStore(const InjectedFailureStore&) = delete;
  InjectedFailureStore(InjectedFailureStore&&) = delete;
  InjectedFailureStore& operator=(const InjectedFailureStore&) = delete;
  InjectedFailureStore& operator=(InjectedFailureStore&&) = delete;

  bool reject_resize = false;
  usize failing_read = 0;
  mutable std::vector<std::pair<usize, usize>> reads;

  Result<> resizeTuples(const ShapeType& shape) override
  {
    return reject_resize ? MakeErrorResult(k_InjectedError, "injected resize failure") : DataStore<T>::resizeTuples(shape);
  }

  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    reads.emplace_back(start, buffer.size());
    return reads.size() == failing_read ? MakeErrorResult(k_InjectedError, "injected face read failure") : DataStore<T>::copyIntoBuffer(start, buffer);
  }
};

struct SupportSnapshot
{
  SupportSnapshot() = default;
  ~SupportSnapshot() noexcept = default;
  SupportSnapshot(const SupportSnapshot&) = default;
  SupportSnapshot(SupportSnapshot&&) noexcept = default;
  SupportSnapshot& operator=(const SupportSnapshot&) = default;
  SupportSnapshot& operator=(SupportSnapshot&&) noexcept = default;

  std::optional<DataObject::IdType> id;
  std::optional<DataObject::IdType> association;
  const SupportListType* object = nullptr;
  std::vector<DataPath> Paths;
  std::vector<std::vector<SupportIndexType>> Lists;
};

DataPath SupportDataPath(bool neighbors)
{
  return k_MeshGeomPath.createChildPath(neighbors ? TriangleGeom::k_EltNeighbors : TriangleGeom::k_EltsContainingVert);
}

SupportSnapshot CaptureSupportSnapshot(const DataStructure& dataStructure, bool neighbors)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  SupportSnapshot snapshot;
  snapshot.association = neighbors ? geometry.getElementNeighborsId() : geometry.getElementContainingVertId();
  snapshot.object = dataStructure.getDataAs<SupportListType>(SupportDataPath(neighbors));
  if(snapshot.object != nullptr)
  {
    snapshot.id = snapshot.object->getId();
    snapshot.Paths = snapshot.object->getDataPaths();
    for(usize i = 0; i < snapshot.object->size(); ++i)
    {
      std::vector<SupportIndexType> list;
      list.reserve(snapshot.object->getNumberOfElements(i));
      for(usize j = 0; j < snapshot.object->getNumberOfElements(i); ++j)
      {
        list.push_back(snapshot.object->getElementListPointer(i)[j]);
      }
      snapshot.Lists.push_back(std::move(list));
    }
  }
  return snapshot;
}

void RequireSupportSnapshot(const DataStructure& dataStructure, bool neighbors, const SupportSnapshot& expected)
{
  const auto actual = CaptureSupportSnapshot(dataStructure, neighbors);
  CHECK(actual.association == expected.association);
  REQUIRE(actual.id == expected.id);
  CHECK(actual.object == expected.object);
  CHECK(actual.Paths == expected.Paths);
  CHECK(actual.Lists == expected.Lists);
}

void CreateSupportData(DataStructure& dataStructure, bool neighbors, const std::vector<std::vector<SupportIndexType>>& lists, bool associate = true)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  auto* supportPtr = SupportListType::Create(dataStructure, neighbors ? TriangleGeom::k_EltNeighbors : TriangleGeom::k_EltsContainingVert, geometry.getId());
  REQUIRE(supportPtr != nullptr);
  supportPtr->allocateLists(std::vector<uint16>(lists.size(), 0));
  for(usize i = 0; i < lists.size(); ++i)
  {
    auto list = lists[i];
    if(!list.empty())
    {
      REQUIRE(supportPtr->setElementList(i, static_cast<uint16>(list.size()), list.data()));
    }
  }
  if(associate)
  {
    if(neighbors)
    {
      geometry.setElementNeighborsId(supportPtr->getId());
    }
    else
    {
      geometry.setElementContainingVertId(supportPtr->getId());
    }
  }
  REQUIRE(CaptureSupportSnapshot(dataStructure, neighbors).Lists == lists);
}

template <typename T>
DataArray<T>& CreateSupportFixtureArray(DataStructure& dataStructure, const DataPath& path, usize tuples, usize components, DataObject::IdType parent)
{
  auto store = data_store_utilities::CreateDataStore<T>(dataStructure, path, {tuples}, {components});
  REQUIRE(store != nullptr);
  store->fill(T{});
  auto* arrayPtr = DataArray<T>::Create(dataStructure, path.getTargetName(), std::move(store), parent);
  REQUIRE(arrayPtr != nullptr);
  return *arrayPtr;
}

void ApplySupportFixtureActions(DataStructure& dataStructure, const IFilter& filter, const Arguments& args)
{
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto applied = preflight.output_actions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
}

void RequireInjectedFailure(const Result<>& result, const std::string& message)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == k_InjectedError);
  CHECK(result.errors()[0].message == message);
}

void RequireSupportFixtureOocStores(const DataStructure& dataStructure, const std::vector<DataPath>& paths)
{
  for(const auto& path : paths)
  {
    CAPTURE(path);
    const auto* arrayPtr = dataStructure.getDataAs<IDataArray>(path);
    REQUIRE(arrayPtr != nullptr);
    REQUIRE(arrayPtr->getDataFormat() == "HDF5-OOC");
    REQUIRE(arrayPtr->getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  }
}

// A fresh destination stays separate from the backing stores.
class SupportSaveFiles
{
public:
  explicit SupportSaveFiles(const std::string& label)
  {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for(usize i = 0; i < Paths.size(); ++i)
    {
      Paths[i] = fs::temp_directory_path() / fmt::format("support-ownership-{}-{}-{}.dream3d", label, nonce, i);
      REQUIRE_FALSE(fs::exists(Paths[i]));
    }
  }
  ~SupportSaveFiles() noexcept
  {
    for(const auto& path : Paths)
    {
      std::error_code error;
      fs::remove(path, error);
    }
  }
  SupportSaveFiles(const SupportSaveFiles&) = delete;
  SupportSaveFiles(SupportSaveFiles&&) = delete;
  SupportSaveFiles& operator=(const SupportSaveFiles&) = delete;
  SupportSaveFiles& operator=(SupportSaveFiles&&) = delete;
  std::array<fs::path, 1> Paths;
};

} // namespace

namespace
{
const std::array k_CurvatureOutputKeys = {FeatureFaceCurvatureFilter::k_PrincipalCurvature1Path_Key, FeatureFaceCurvatureFilter::k_PrincipalCurvature2Path_Key,
                                          FeatureFaceCurvatureFilter::k_GaussianCurvaturePath_Key,   FeatureFaceCurvatureFilter::k_MeanCurvaturePath_Key,
                                          FeatureFaceCurvatureFilter::k_WeingartenMatrixPath_Key,    FeatureFaceCurvatureFilter::k_PrincipalDirection1Path_Key,
                                          FeatureFaceCurvatureFilter::k_PrincipalDirection2Path_Key};

DataPath CurvatureOutputPath(usize index)
{
  return k_MeshFacePath.createChildPath("Output" + std::to_string(index));
}

Arguments CreateCurvatureFixture(DataStructure& dataStructure, bool incidence, bool optionalOutputs, bool neighbors = false)
{
  auto* geometryPtr = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geometryPtr != nullptr);
  auto* faceDataPtr = AttributeMatrix::Create(dataStructure, "FaceData", {9}, geometryPtr->getId());
  REQUIRE(faceDataPtr != nullptr);
  geometryPtr->setFaceAttributeMatrix(*faceDataPtr);
  auto& vertices = CreateSupportFixtureArray<float32>(dataStructure, k_MeshGeomPath.createChildPath("Vertices"), 19, 3, geometryPtr->getId());
  auto& faces = CreateSupportFixtureArray<SupportIndexType>(dataStructure, k_MeshGeomPath.createChildPath("Faces"), 9, 3, geometryPtr->getId());
  geometryPtr->setVertices(vertices);
  geometryPtr->setFaceList(faces);
  auto& labels = CreateSupportFixtureArray<int32>(dataStructure, k_MeshFacePath.createChildPath("Labels"), 9, 2, faceDataPtr->getId());
  auto& ids = CreateSupportFixtureArray<int32>(dataStructure, k_MeshFacePath.createChildPath("Ids"), 9, 1, faceDataPtr->getId());
  auto& normals = CreateSupportFixtureArray<float64>(dataStructure, k_MeshFacePath.createChildPath("Normals"), 9, 3, faceDataPtr->getId());
  auto& centroids = CreateSupportFixtureArray<float64>(dataStructure, k_MeshFacePath.createChildPath("Centroids"), 9, 3, faceDataPtr->getId());
  for(usize t = 0; t < 9; ++t)
  {
    const float64 x = 1.0 + static_cast<float64>(t % 3);
    const float64 y = 1.0 + static_cast<float64>((t / 3) * (t / 3));
    vertices.setComponent(2 * t + 1, 0, static_cast<float32>(1.5 * x + y));
    vertices.setComponent(2 * t + 1, 1, static_cast<float32>(1.5 * y - x));
    vertices.setComponent(2 * t + 2, 0, static_cast<float32>(1.5 * x - y));
    vertices.setComponent(2 * t + 2, 1, static_cast<float32>(1.5 * y + x));
    faces.setComponent(t, 1, 2 * t + 1);
    faces.setComponent(t, 2, 2 * t + 2);
    labels.setComponent(t, 0, 11);
    labels.setComponent(t, 1, 13);
    ids[t] = 1;
    normals.setComponent(t, 2, 1.0);
    centroids.setComponent(t, 0, x);
    centroids.setComponent(t, 1, y);
  }
  if(incidence)
  {
    // Literal independent vertex incidence, in vertex order 0..18.
    CreateSupportData(dataStructure, false, {{0, 1, 2, 3, 4, 5, 6, 7, 8}, {0}, {0}, {1}, {1}, {2}, {2}, {3}, {3}, {4}, {4}, {5}, {5}, {6}, {6}, {7}, {7}, {8}, {8}});
  }
  if(neighbors)
  {
    CreateSupportData(dataStructure, true, {{}, {}, {}, {}, {}, {}, {}, {}, {}});
  }
  Arguments args;
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(k_MeshGeomPath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceAttribMatrixPath_Key, std::make_any<DataPath>(k_MeshFacePath));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_NeighborhoodRing_Key, std::make_any<int32>(1));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_UseFaceNormals_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputePrincipalDirection_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeGaussianCurvature_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeMeanCurvaturePath_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_ComputeWeingartenMatrix_Key, std::make_any<bool>(optionalOutputs));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceLabelsPath_Key, std::make_any<DataPath>(k_MeshFacePath.createChildPath("Labels")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FeatureFaceIdsPath_Key, std::make_any<DataPath>(k_MeshFacePath.createChildPath("Ids")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceNormalsPath_Key, std::make_any<DataPath>(k_MeshFacePath.createChildPath("Normals")));
  args.insertOrAssign(FeatureFaceCurvatureFilter::k_FaceCentroidsPath_Key, std::make_any<DataPath>(k_MeshFacePath.createChildPath("Centroids")));
  for(usize i = 0; i < k_CurvatureOutputKeys.size(); ++i)
  {
    args.insertOrAssign(k_CurvatureOutputKeys[i], std::make_any<DataPath>(CurvatureOutputPath(i)));
  }
  return args;
}

Result<> RunCurvature(DataStructure& dataStructure, bool cancelled)
{
  FeatureFaceCurvatureInputValues inputs{};
  inputs.n_ring_count = 1;
  inputs.triangle_geom_path = k_MeshGeomPath;
  inputs.surface_mesh_principal_curvature1_path = CurvatureOutputPath(0);
  inputs.surface_mesh_feature_face_ids_path = k_MeshFacePath.createChildPath("Ids");
  inputs.surface_mesh_face_labels_path = k_MeshFacePath.createChildPath("Labels");
  inputs.surface_mesh_face_normals_path = k_MeshFacePath.createChildPath("Normals");
  inputs.surface_mesh_triangle_centroids_path = k_MeshFacePath.createChildPath("Centroids");
  inputs.use_normals_for_curve_fitting = true;
  inputs.surface_mesh_principal_curvature1s_path = CurvatureOutputPath(0);
  inputs.surface_mesh_principal_curvature2s_path = CurvatureOutputPath(1);
  inputs.surface_mesh_gaussian_curvatures_path = CurvatureOutputPath(2);
  inputs.surface_mesh_mean_curvatures_path = CurvatureOutputPath(3);
  inputs.surface_mesh_weingarten_matrix_path = CurvatureOutputPath(4);
  inputs.surface_mesh_principal_direction1s_path = CurvatureOutputPath(5);
  inputs.surface_mesh_principal_direction2s_path = CurvatureOutputPath(6);
  const std::atomic_bool cancel = cancelled;
  return FeatureFaceCurvature(dataStructure, {}, cancel, &inputs)();
}

void CheckCurvatureInputs(const DataStructure& dataStructure)
{
  const auto* verticesPtr = dataStructure.getDataAs<Float32ArrayType>(k_MeshGeomPath.createChildPath("Vertices"));
  const auto* facesPtr = dataStructure.getDataAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces"));
  const auto* labelsPtr = dataStructure.getDataAs<Int32ArrayType>(k_MeshFacePath.createChildPath("Labels"));
  const auto* idsPtr = dataStructure.getDataAs<Int32ArrayType>(k_MeshFacePath.createChildPath("Ids"));
  const auto* normalsPtr = dataStructure.getDataAs<Float64ArrayType>(k_MeshFacePath.createChildPath("Normals"));
  const auto* centroidsPtr = dataStructure.getDataAs<Float64ArrayType>(k_MeshFacePath.createChildPath("Centroids"));
  REQUIRE(verticesPtr != nullptr);
  REQUIRE(facesPtr != nullptr);
  REQUIRE(labelsPtr != nullptr);
  REQUIRE(idsPtr != nullptr);
  REQUIRE(normalsPtr != nullptr);
  REQUIRE(centroidsPtr != nullptr);
  REQUIRE(verticesPtr->getNumberOfTuples() == 19);
  REQUIRE(facesPtr->getNumberOfTuples() == 9);
  for(usize c = 0; c < 3; ++c)
  {
    CHECK(verticesPtr->getDataStoreRef().getComponentValue(0, c) == 0.0F);
  }
  for(usize t = 0; t < 9; ++t)
  {
    const float64 x = 1.0 + static_cast<float64>(t % 3);
    const float64 y = 1.0 + static_cast<float64>((t / 3) * (t / 3));
    const std::array<float32, 6> expectedVertices = {static_cast<float32>(1.5 * x + y), static_cast<float32>(1.5 * y - x), 0, static_cast<float32>(1.5 * x - y), static_cast<float32>(1.5 * y + x), 0};
    for(usize i = 0; i < expectedVertices.size(); ++i)
    {
      CHECK((*verticesPtr)[(2 * t + 1) * 3 + i] == expectedVertices[i]);
    }
    CHECK(facesPtr->getDataStoreRef().getComponentValue(t, 0) == 0);
    CHECK(facesPtr->getDataStoreRef().getComponentValue(t, 1) == 2 * t + 1);
    CHECK(facesPtr->getDataStoreRef().getComponentValue(t, 2) == 2 * t + 2);
    CHECK(labelsPtr->getDataStoreRef().getComponentValue(t, 0) == 11);
    CHECK(labelsPtr->getDataStoreRef().getComponentValue(t, 1) == 13);
    CHECK((*idsPtr)[t] == 1);
    const std::array<float64, 3> expectedNormal = {0, 0, 1};
    const std::array<float64, 3> expectedCentroid = {x, y, 0};
    for(usize c = 0; c < 3; ++c)
    {
      CHECK(normalsPtr->getDataStoreRef().getComponentValue(t, c) == expectedNormal[c]);
      CHECK(centroidsPtr->getDataStoreRef().getComponentValue(t, c) == expectedCentroid[c]);
    }
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath));
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_MeshGeomPath);
  CHECK(geometry.getVertices() == verticesPtr);
  CHECK(geometry.getFaces() == facesPtr);
}

std::vector<float64> CheckCurvatureOutputs(const DataStructure& dataStructure, bool optionalOutputs)
{
  std::vector<float64> values;
  for(usize i = 0; i < (optionalOutputs ? k_CurvatureOutputKeys.size() : 2); ++i)
  {
    const auto* outputPtr = dataStructure.getDataAs<Float64ArrayType>(CurvatureOutputPath(i));
    REQUIRE(outputPtr != nullptr);
    REQUIRE(outputPtr->getNumberOfTuples() == 9);
    for(usize j = 0; j < outputPtr->getSize(); ++j)
    {
      const float64 value = (*outputPtr)[j];
      // A plane has zero Hessian. Direction signs remain unconstrained.
      if(i < 5)
      {
        CHECK(value == Approx(0.0).margin(1.0e-12));
      }
      else
      {
        CHECK(std::isfinite(value));
      }
      values.push_back(value);
    }
  }
  return values;
}

void CheckCurvatureSave(DataStructure& dataStructure, const SupportSnapshot& incidence)
{
  SupportSaveFiles files("curvature");
  const auto outputs = CheckCurvatureOutputs(dataStructure, false);
  // The writer omits supplied connectivity and leaves the caller's objects in place.
  const auto save = hdf5::DataStructureWriter::WriteFile(dataStructure, files.Paths[0]);
  SIMPLNX_RESULT_REQUIRE_VALID(save);
  RequireSupportSnapshot(dataStructure, false, incidence);
  CheckCurvatureInputs(dataStructure);
  CHECK(CheckCurvatureOutputs(dataStructure, false) == outputs);
  const auto read = hdf5::DataStructureReader::ReadFile(files.Paths[0]);
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  CheckCurvatureInputs(read.value());
  CHECK(CheckCurvatureOutputs(read.value(), false) == outputs);
  RequireSupportSnapshot(read.value(), false, {});
  RequireSupportSnapshot(read.value(), true, {});
  unit_test::CheckArraysInheritTupleDims(read.value());
}

void CheckCurvatureSuccess(bool incidence, bool cancelled, bool optionalOutputs, bool save, bool ooc)
{
  DataStructure dataStructure;
  const auto args = CreateCurvatureFixture(dataStructure, incidence, optionalOutputs, !save);
  const auto incidenceBefore = CaptureSupportSnapshot(dataStructure, false);
  const auto neighborsBefore = CaptureSupportSnapshot(dataStructure, true);
  FeatureFaceCurvatureFilter filter;
  if(cancelled)
  {
    ApplySupportFixtureActions(dataStructure, filter, args);
    const auto result = RunCurvature(dataStructure, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
    const auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    CheckCurvatureOutputs(dataStructure, optionalOutputs);
  }
  CheckCurvatureInputs(dataStructure);
  RequireSupportSnapshot(dataStructure, true, neighborsBefore);
  if(ooc)
  {
    std::vector<DataPath> paths = {k_MeshGeomPath.createChildPath("Vertices"), k_MeshGeomPath.createChildPath("Faces"),   k_MeshFacePath.createChildPath("Labels"),
                                   k_MeshFacePath.createChildPath("Ids"),      k_MeshFacePath.createChildPath("Normals"), k_MeshFacePath.createChildPath("Centroids")};
    for(usize i = 0; i < (optionalOutputs ? k_CurvatureOutputKeys.size() : 2); ++i)
    {
      paths.push_back(CurvatureOutputPath(i));
    }
    RequireSupportFixtureOocStores(dataStructure, paths);
  }
  if(save)
  {
    CheckCurvatureSave(dataStructure, incidenceBefore);
  }
  else
  {
    RequireSupportSnapshot(dataStructure, false, incidenceBefore);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: support ownership", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  CAPTURE(incidence);
  CheckCurvatureSuccess(incidence, false, false, false, false);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: finite planar control", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  CheckCurvatureSuccess(false, false, true, false, false);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: cancelled cleanup", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  CAPTURE(incidence);
  CheckCurvatureSuccess(incidence, true, false, false, false);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: support creation failure", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  DataStructure dataStructure;
  const auto args = CreateCurvatureFixture(dataStructure, false, false);
  ApplySupportFixtureActions(dataStructure, FeatureFaceCurvatureFilter{}, args);
  for(usize i = 0; i < 2; ++i)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(CurvatureOutputPath(i)));
    dataStructure.getDataRefAs<Float64ArrayType>(CurvatureOutputPath(i)).fill(73.0);
  }
  auto store = std::make_shared<InjectedFailureStore<SupportIndexType>>(ShapeType{9}, ShapeType{3}, 0);
  for(usize t = 0; t < 9; ++t)
  {
    (*store)[3 * t + 1] = 2 * t + 1;
    (*store)[3 * t + 2] = 2 * t + 2;
  }
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces")));
  const auto replacement = dataStructure.getDataRefAs<DataArray<SupportIndexType>>(k_MeshGeomPath.createChildPath("Faces")).setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replacement);
  store->reads.clear();
  store->failing_read = 1;
  const auto result = RunCurvature(dataStructure, false);
  RequireInjectedFailure(result, "injected face read failure");
  const std::vector<std::pair<usize, usize>> expectedReads = {{0, 27}};
  REQUIRE(store->reads == expectedReads);
  RequireSupportSnapshot(dataStructure, false, {});
  RequireSupportSnapshot(dataStructure, true, {});
  for(usize i = 0; i < 2; ++i)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(CurvatureOutputPath(i)));
    const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(CurvatureOutputPath(i));
    for(usize t = 0; t < output.getSize(); ++t)
    {
      CHECK(output[t] == 73.0);
    }
  }
  CheckCurvatureInputs(dataStructure);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: unassociated named child", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  DataStructure dataStructure;
  const auto args = CreateCurvatureFixture(dataStructure, false, false);
  CreateSupportData(dataStructure, false, {{42, 7}}, false);
  const auto before = CaptureSupportSnapshot(dataStructure, false);
  ApplySupportFixtureActions(dataStructure, FeatureFaceCurvatureFilter{}, args);
  const auto result = RunCurvature(dataStructure, false);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -2231);
  RequireSupportSnapshot(dataStructure, false, before);
  RequireSupportSnapshot(dataStructure, true, {});
  CheckCurvatureInputs(dataStructure);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: retained support save compatibility", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 1);
  const bool incidence = GENERATE(false, true);
  CAPTURE(incidence);
  CheckCurvatureSuccess(incidence, false, false, true, false);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: real store support ownership", "[SimplnxCore][.OocStoreContract]")
{
  REQUIRE(Application::GetOrCreateInstance() != nullptr);
  REQUIRE(Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr);
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceOutOfCore, 1);
  const bool incidence = GENERATE(false, true);
  CAPTURE(incidence);
  CheckCurvatureSuccess(incidence, false, false, true, true);
  std::cout << fmt::format(
      "Curvature ownership/save/recovery complete incidence={} vertices=HDF5-OOC faces=HDF5-OOC labels=HDF5-OOC ids=HDF5-OOC normals=HDF5-OOC centroids=HDF5-OOC outputs=HDF5-OOC\n", incidence);
}

namespace
{
void CheckMultiFeatureCurvature(bool diskBacked, bool optionalOutputs)
{
  const unit_test::PreferencesSentinel preferences(diskBacked ? DataStorageMode::ForceOutOfCore : DataStorageMode::ForceInCore, 1);
  constexpr usize featureCount = 8;
  constexpr usize facesPerFeature = 9;
  constexpr usize faceCount = featureCount * facesPerFeature;
  const DataPath geomPath({"Triangles"});
  const DataPath facePath = geomPath.createChildPath("FaceData");
  DataStructure dataStructure;
  auto* geom = TriangleGeom::Create(dataStructure, "Triangles");
  REQUIRE(geom != nullptr);
  auto* faceData = AttributeMatrix::Create(dataStructure, "FaceData", {faceCount}, geom->getId());
  REQUIRE(faceData != nullptr);
  geom->setFaceAttributeMatrix(*faceData);
  auto* vertices =
      Float32ArrayType::Create(dataStructure, "Vertices", data_store_utilities::CreateDataStore<float32>(dataStructure, geomPath.createChildPath("Vertices"), {featureCount * 19}, {3}), geom->getId());
  auto* faces = DataArray<IGeometry::MeshIndexType>::Create(
      dataStructure, "Faces", data_store_utilities::CreateDataStore<IGeometry::MeshIndexType>(dataStructure, geomPath.createChildPath("Faces"), {faceCount}, {3}), geom->getId());
  REQUIRE(vertices != nullptr);
  REQUIRE(faces != nullptr);
  geom->setVertices(*vertices);
  geom->setFaceList(*faces);
  auto* labels = Int32ArrayType::Create(dataStructure, "Labels", data_store_utilities::CreateDataStore<int32>(dataStructure, facePath.createChildPath("Labels"), {faceCount}, {2}), faceData->getId());
  auto* ids = Int32ArrayType::Create(dataStructure, "Ids", data_store_utilities::CreateDataStore<int32>(dataStructure, facePath.createChildPath("Ids"), {faceCount}, {1}), faceData->getId());
  auto* normals =
      Float64ArrayType::Create(dataStructure, "Normals", data_store_utilities::CreateDataStore<float64>(dataStructure, facePath.createChildPath("Normals"), {faceCount}, {3}), faceData->getId());
  auto* centroids =
      Float64ArrayType::Create(dataStructure, "Centroids", data_store_utilities::CreateDataStore<float64>(dataStructure, facePath.createChildPath("Centroids"), {faceCount}, {3}), faceData->getId());
  REQUIRE(labels != nullptr);
  REQUIRE(ids != nullptr);
  REQUIRE(normals != nullptr);
  REQUIRE(centroids != nullptr);
  for(usize f = 0; f < featureCount; ++f)
  {
    const usize base = f * 19;
    vertices->setComponent(base, 0, 0.0F);
    vertices->setComponent(base, 1, 0.0F);
    vertices->setComponent(base, 2, static_cast<float32>(f));
    for(usize t = 0; t < facesPerFeature; ++t)
    {
      const usize face = f * facesPerFeature + t;
      const float64 x = 1.0 + static_cast<float64>(t % 3);
      const float64 y = 1.0 + static_cast<float64>((t / 3) * (t / 3));
      // Each feature is a disjoint patch of the plane z=f. All nine triangles
      // share a vertex, have centroid (x,y,f), and normal +Z.
      for(usize side = 0; side < 2; ++side)
      {
        const float64 sign = side == 0 ? 1.0 : -1.0;
        vertices->setComponent(base + 2 * t + 1 + side, 0, static_cast<float32>(1.5 * x + sign * y));
        vertices->setComponent(base + 2 * t + 1 + side, 1, static_cast<float32>(1.5 * y - sign * x));
        vertices->setComponent(base + 2 * t + 1 + side, 2, static_cast<float32>(f));
      }
      faces->setComponent(face, 0, base);
      faces->setComponent(face, 1, base + 2 * t + 1);
      faces->setComponent(face, 2, base + 2 * t + 2);
      labels->setComponent(face, 0, static_cast<int32>(2 * f + 1));
      labels->setComponent(face, 1, static_cast<int32>(2 * f + 2));
      ids->setComponent(face, 0, static_cast<int32>(f + 1));
      normals->setComponent(face, 0, 0.0);
      normals->setComponent(face, 1, 0.0);
      normals->setComponent(face, 2, 1.0);
      centroids->setComponent(face, 0, x);
      centroids->setComponent(face, 1, y);
      centroids->setComponent(face, 2, static_cast<float64>(f));
    }
  }
  for(const IDataArray* array : std::array<const IDataArray*, 6>{vertices, faces, labels, ids, normals, centroids})
  {
    REQUIRE((array->getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore) == diskBacked);
    if(diskBacked)
    {
      REQUIRE(array->getIDataStoreRef().getDataFormat() == "HDF5-OOC");
    }
  }
  // Run the algorithm on output arrays that already hold a sentinel. The expected curvatures of a plane are 0, the
  // same value an unwritten store returns, so the sentinel makes every lost or skipped write fail the checks below.
  constexpr float64 k_Sentinel = 999.0;
  constexpr usize k_OutputCount = 7;
  const std::array<usize, k_OutputCount> outputComponents = {1, 1, 1, 1, 4, 3, 3};
  std::array<DataPath, k_OutputCount> outputPaths;
  for(usize i = 0; i < k_OutputCount; ++i)
  {
    outputPaths[i] = facePath.createChildPath("Output" + std::to_string(i));
    if(i >= 2 && !optionalOutputs)
    {
      continue;
    }
    auto* output = Float64ArrayType::Create(dataStructure, outputPaths[i].getTargetName(),
                                            data_store_utilities::CreateDataStore<float64>(dataStructure, outputPaths[i], {faceCount}, {outputComponents[i]}), faceData->getId());
    REQUIRE(output != nullptr);
    output->fill(k_Sentinel);
  }
  FeatureFaceCurvatureInputValues inputValues;
  inputValues.n_ring_count = 1;
  inputValues.use_normals_for_curve_fitting = true;
  inputValues.triangle_geom_path = geomPath;
  inputValues.surface_mesh_face_labels_path = facePath.createChildPath("Labels");
  inputValues.surface_mesh_feature_face_ids_path = facePath.createChildPath("Ids");
  inputValues.surface_mesh_face_normals_path = facePath.createChildPath("Normals");
  inputValues.surface_mesh_triangle_centroids_path = facePath.createChildPath("Centroids");
  inputValues.surface_mesh_principal_curvature1_path = outputPaths[0];
  inputValues.surface_mesh_principal_curvature1s_path = outputPaths[0];
  inputValues.surface_mesh_principal_curvature2s_path = outputPaths[1];
  inputValues.surface_mesh_gaussian_curvatures_path = outputPaths[2];
  inputValues.surface_mesh_mean_curvatures_path = outputPaths[3];
  inputValues.surface_mesh_weingarten_matrix_path = outputPaths[4];
  inputValues.surface_mesh_principal_direction1s_path = outputPaths[5];
  inputValues.surface_mesh_principal_direction2s_path = outputPaths[6];
  // This existing status message is emitted only when tasks are queued to TBB.
  // It witnesses the gate decision without relying on a race corrupting data.
  std::atomic_bool queued = false;
  const IFilter::MessageHandler handler{[&](const IFilter::Message& message) {
    if(message.message.find("Feature Faces to the work queue") != std::string::npos)
    {
      queued = true;
    }
  }};
  const std::atomic_bool shouldCancel = false;
  const auto result = FeatureFaceCurvature(dataStructure, handler, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_VALID(result);
#ifdef SIMPLNX_ENABLE_MULTICORE
  CHECK(queued.load() == !diskBacked);
#else
  CHECK_FALSE(queued.load());
#endif
  for(usize i = 0; i < (optionalOutputs ? k_OutputCount : 2); ++i)
  {
    const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(outputPaths[i]);
    REQUIRE((output.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore) == diskBacked);
    if(diskBacked)
    {
      REQUIRE(output.getDataStoreRef().getDataFormat() == "HDF5-OOC");
    }
    for(usize t = 0; t < faceCount; ++t)
    {
      CAPTURE(i, t);
      if(i < 5)
      {
        // Independent oracle: a plane has zero Hessian, hence zero principal,
        // Gaussian and mean curvatures and a zero Weingarten matrix everywhere.
        for(usize c = 0; c < output.getNumberOfComponents(); ++c)
        {
          CHECK(output.getDataStoreRef().getComponentValue(t, c) == Approx(0.0).margin(1.0e-12));
        }
      }
      else
      {
        // Repeated eigenvalues do not define a unique direction. Both returned
        // directions must still be unit vectors in the plane (up to sign).
        const float64 x = output.getDataStoreRef().getComponentValue(t, 0);
        const float64 y = output.getDataStoreRef().getComponentValue(t, 1);
        CHECK(x * x + y * y == Approx(1.0).margin(1.0e-12));
        CHECK(output.getDataStoreRef().getComponentValue(t, 2) == Approx(0.0).margin(1.0e-12));
      }
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: multi-feature planes", "[SimplnxCore][FeatureFaceCurvatureFilter]")
{
  // Storage-aware array creation and the storage preferences need the application that LoadPlugins() creates.
  unit_test::LoadPlugins();
  const bool optionalOutputs = GENERATE(false, true);
  CheckMultiFeatureCurvature(false, optionalOutputs);
}

TEST_CASE("SimplnxCore::FeatureFaceCurvatureFilter: multi-feature planes with real OOC stores", "[SimplnxCore][FeatureFaceCurvatureFilter][OOC]")
{
  unit_test::LoadPlugins();
  if(!data_store_utilities::GetIOCollection().hasDataStoreCreationFunction("HDF5-OOC"))
  {
#if SIMPLNX_TEST_ALGORITHM_PATH == 1
    FAIL("The OOC-only build must register HDF5-OOC storage.");
#else
    WARN("Real-store body requires an OOC-enabled build.");
    return;
#endif
  }
  const bool optionalOutputs = GENERATE(false, true);
  CheckMultiFeatureCurvature(true, optionalOutputs);
}

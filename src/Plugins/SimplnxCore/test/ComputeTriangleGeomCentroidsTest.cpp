#include "SimplnxCore/Filters/ComputeTriangleGeomCentroidsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace ComputeTriangleGeomCentroidsFilterTest
{
const std::string k_TriangleGeometryName = "TriangleDataContainer";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_FaceFeatureName = "FaceFeatureData";
const std::string k_FaceAttributeMatrixName = "FaceData";
const std::string k_CentroidsArrayName = "Centroids [NX Computed]";

const DataPath k_GeometryPath = DataPath({k_TriangleGeometryName});
const DataPath k_FeatureAttributeMatrixPath = k_GeometryPath.createChildPath(k_FaceFeatureName);
const DataPath k_FaceLabelsPath = k_GeometryPath.createChildPath(k_FaceAttributeMatrixName).createChildPath(k_FaceLabelsName);

} // namespace ComputeTriangleGeomCentroidsFilterTest

using namespace ComputeTriangleGeomCentroidsFilterTest;

TEST_CASE("SimplnxCore::ComputeTriangleGeomCentroids", "[SimplnxCore][ComputeTriangleGeomCentroids]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "12_IN625_GBCD.tar.gz", "12_IN625_GBCD");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/12_IN625_GBCD/12_IN625_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    // Instantiate the filter and an Arguments Object
    ComputeTriangleGeomCentroidsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAttributeMatrixPath));
    // Output Path
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CentroidsArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the results
  {
    const std::string kExemplarArrayName = "Centroids";
    const DataPath kExemplarArrayPath = k_FeatureAttributeMatrixPath.createChildPath(kExemplarArrayName);
    const DataPath kNxArrayPath = k_FeatureAttributeMatrixPath.createChildPath(k_CentroidsArrayName);

    const auto& kExemplarsArray = dataStructure.getDataRefAs<IDataArray>(kExemplarArrayPath);
    const auto& kNxArray = dataStructure.getDataRefAs<IDataArray>(kNxArrayPath);

    CompareDataArrays<float32>(kExemplarsArray, kNxArray);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_triangle_geom_centroids.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
// Builds a tiny triangle mesh on a periodic domain X in [0, 4]. All vertices sit at z = 0 and each feature
// occupies a distinct y value, so ONLY the X axis can trip the periodic (both-opposing-faces) condition;
// the whole-geometry bounding box is exactly x=[0,4], y=[1,3], z=[0,0]. Face labels group triangles into
// four features that exercise the periodic centroid, plus feature 0 which is empty:
//   F1 non-wrapping     x = {1, 2, 3}          -> touches neither x-face, centroid unchanged
//   F2 symmetric wrap   x = {0, 0, 4, 4}       -> mass split evenly across the seam
//   F3 asymmetric wrap  x = {0, 3, 3.5, 4}     -> bug-killer: the old constant offset lands at 4.625 (out of bounds)
//   F4 domain-filling   x = {0, 1, 2, 3, 4}    -> degenerate, falls back to the arithmetic mean
DataStructure BuildPeriodicToyMesh()
{
  DataStructure dataStructure;
  auto& triangleGeom = *TriangleGeom::Create(dataStructure, k_TriangleGeometryName);

  // 16 vertices (x, y, z). Indices 0-2 -> F1, 3-6 -> F2, 7-10 -> F3, 11-15 -> F4.
  const std::vector<float32> vertices = {1.0F, 1.0F, 0.0F, 2.0F, 1.0F, 0.0F, 3.0F, 1.0F, 0.0F,               // F1
                                         0.0F, 2.0F, 0.0F, 0.0F, 2.0F, 0.0F, 4.0F, 2.0F, 0.0F, 4.0F, 2.0F, 0.0F, // F2
                                         0.0F, 3.0F, 0.0F, 3.0F, 3.0F, 0.0F, 3.5F, 3.0F, 0.0F, 4.0F, 3.0F, 0.0F, // F3
                                         0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 2.0F, 1.0F, 0.0F, 3.0F, 1.0F, 0.0F, 4.0F, 1.0F, 0.0F}; // F4
  const usize numVertices = vertices.size() / 3;

  // 8 triangles. Each feature's triangles share vertices so the union of their vertices is the feature set.
  const std::vector<IGeometry::MeshIndexType> faces = {0, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13, 12, 13, 14, 13, 14, 15};
  const usize numFaces = faces.size() / 3;

  auto* vertexAttrMat = AttributeMatrix::Create(dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, {numVertices}, triangleGeom.getId());
  triangleGeom.setVertexAttributeMatrix(*vertexAttrMat);
  auto* faceAttrMat = AttributeMatrix::Create(dataStructure, INodeGeometry2D::k_FaceAttributeMatrixName, {numFaces}, triangleGeom.getId());
  triangleGeom.setFaceAttributeMatrix(*faceAttrMat);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{numVertices}, std::vector<usize>{3}, 0.0F);
  auto* vertexList = IGeometry::SharedVertexList::Create(dataStructure, "Vertices", std::move(vertexStore), vertexAttrMat->getId());
  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{numFaces}, std::vector<usize>{3}, 0);
  auto* faceList = IGeometry::SharedFaceList::Create(dataStructure, "SharedTriList", std::move(faceStore), faceAttrMat->getId());

  auto vertexStoreRef = vertexList->getDataStorePtr().lock();
  for(usize i = 0; i < vertices.size(); i++)
  {
    vertexStoreRef->setValue(i, vertices[i]);
  }
  auto faceStoreRef = faceList->getDataStorePtr().lock();
  for(usize i = 0; i < faces.size(); i++)
  {
    faceStoreRef->setValue(i, faces[i]);
  }
  triangleGeom.setVertices(*vertexList);
  triangleGeom.setFaceList(*faceList);

  // FaceLabels (2 components/triangle). Second label is 0 (ignored by the filter). Feature IDs 1..4.
  const std::vector<int32> faceLabels = {1, 0, 2, 0, 2, 0, 3, 0, 3, 0, 4, 0, 4, 0, 4, 0};
  auto* faceDataAttrMat = AttributeMatrix::Create(dataStructure, k_FaceAttributeMatrixName, {numFaces}, triangleGeom.getId());
  auto faceLabelStore = std::make_unique<DataStore<int32>>(std::vector<usize>{numFaces}, std::vector<usize>{2}, 0);
  auto* faceLabelArray = Int32Array::Create(dataStructure, k_FaceLabelsName, std::move(faceLabelStore), faceDataAttrMat->getId());
  auto faceLabelStoreRef = faceLabelArray->getDataStorePtr().lock();
  for(usize i = 0; i < faceLabels.size(); i++)
  {
    faceLabelStoreRef->setValue(i, faceLabels[i]);
  }

  // Feature AttributeMatrix sized for feature IDs 0..4 so no resize happens during execute.
  AttributeMatrix::Create(dataStructure, k_FaceFeatureName, {5}, triangleGeom.getId());

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::ComputeTriangleGeomCentroids: Periodic Minimum-Image Oracle", "[SimplnxCore][ComputeTriangleGeomCentroids]")
{
  UnitTest::LoadPlugins();

  const DataPath centroidsPath = k_FeatureAttributeMatrixPath.createChildPath(k_CentroidsArrayName);
  constexpr float32 k_Tol = 1.0e-4F;

  // Expected centroids per feature (feature 0 is empty -> default 0,0,0).
  // Non-periodic: plain arithmetic mean of each feature's unique vertices.
  const std::vector<std::array<float32, 3>> nonPeriodicExpected = {
      {0.0F, 0.0F, 0.0F},   // F0 empty
      {2.0F, 1.0F, 0.0F},   // F1
      {2.0F, 2.0F, 0.0F},   // F2 naive x = (0+0+4+4)/4
      {2.625F, 3.0F, 0.0F}, // F3 naive x = (0+3+3.5+4)/4
      {2.0F, 1.0F, 0.0F}};  // F4 naive x = (0+1+2+3+4)/5

  // Periodic: X component becomes the minimum-image mean on wrapping features; y/z unchanged.
  const std::vector<std::array<float32, 3>> periodicExpected = {
      {0.0F, 0.0F, 0.0F},   // F0 empty
      {2.0F, 1.0F, 0.0F},   // F1 does not span -> unchanged
      {0.0F, 2.0F, 0.0F},   // F2 symmetric wrap -> seam at x=0
      {3.625F, 3.0F, 0.0F}, // F3 asymmetric wrap -> largest-gap mean (old code -> 4.625, out of bounds)
      {2.0F, 1.0F, 0.0F}};  // F4 domain-filling -> arithmetic-mean fallback

  const bool isPeriodic = GENERATE(false, true);
  const auto& expected = isPeriodic ? periodicExpected : nonPeriodicExpected;

  DYNAMIC_SECTION("IsPeriodic = " << (isPeriodic ? "true" : "false"))
  {
    DataStructure dataStructure = BuildPeriodicToyMesh();

    ComputeTriangleGeomCentroidsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_TriGeometryDataPath_Key, std::make_any<GeometrySelectionParameter::ValueType>(k_GeometryPath));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_FaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(k_FeatureAttributeMatrixPath));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_CentroidsArrayName));
    args.insertOrAssign(ComputeTriangleGeomCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(centroidsPath));
    const auto& centroids = dataStructure.getDataRefAs<Float32Array>(centroidsPath);

    const auto& boundingBox = dataStructure.getDataRefAs<TriangleGeom>(k_GeometryPath).getBoundingBox();

    for(usize feature = 0; feature < expected.size(); feature++)
    {
      for(usize comp = 0; comp < 3; comp++)
      {
        INFO(fmt::format("feature {} component {}", feature, comp));
        REQUIRE(std::abs(centroids[feature * 3 + comp] - expected[feature][comp]) < k_Tol);
      }
    }

    // Class 4 invariant: every non-empty feature centroid must lie inside the periodic domain.
    for(usize feature = 1; feature < expected.size(); feature++)
    {
      for(usize comp = 0; comp < 3; comp++)
      {
        const float32 value = centroids[feature * 3 + comp];
        INFO(fmt::format("in-bounds feature {} component {}", feature, comp));
        REQUIRE(value >= boundingBox.getMinPoint()[comp] - k_Tol);
        REQUIRE(value <= boundingBox.getMaxPoint()[comp] + k_Tol);
      }
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

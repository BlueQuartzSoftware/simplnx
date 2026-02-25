#include "SimplnxCore/Filters/Algorithms/InterpolatePointCloudToRegularGrid.hpp"
#include "SimplnxCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cmath>

using namespace nx::core;

namespace
{
const DataPath k_VertexGeomPath({"VertexGeom"});
const DataPath k_ImageGeomPath({"ImageGeom"});
const DataPath k_VertexDataPath = k_VertexGeomPath.createChildPath("VertexData");
const DataPath k_FaceAreasPath = k_VertexDataPath.createChildPath("FaceAreas");
const DataPath k_MaskPath = k_VertexDataPath.createChildPath("Mask");

const std::string k_InterpolatedGroupName = "InterpolatedData";
const DataPath k_InterpGroupPath = k_ImageGeomPath.createChildPath(k_InterpolatedGroupName);

// Convert a linear voxel index (in a 4x4x1 grid with spacing=1, origin=0) to cell-center coordinates
void setVertexCoordsForVoxelIndex(Float32Array& vertices, usize vertexIdx, uint64 voxelIndex, usize dimX = 4, usize dimY = 4)
{
  usize x = voxelIndex % dimX;
  usize y = (voxelIndex / dimX) % dimY;
  usize z = voxelIndex / (dimX * dimY);

  // Place vertex at the center of the voxel (spacing=1, origin=0)
  vertices[vertexIdx * 3 + 0] = static_cast<float32>(x) + 0.5f;
  vertices[vertexIdx * 3 + 1] = static_cast<float32>(y) + 0.5f;
  vertices[vertexIdx * 3 + 2] = static_cast<float32>(z) + 0.5f;
}

// Create a test DataStructure with a 4x4x1 ImageGeom and a VertexGeom with known data.
// Vertex coordinates are set to map to the given voxel indices.
DataStructure createTestDataStructure(const std::vector<uint64>& voxelIndices, const std::vector<float64>& faceAreas, bool allMasked = true, usize maskedOutIndex = 0)
{
  DataStructure ds;
  usize numVertices = voxelIndices.size();

  // Vertex Geometry
  auto* vertexGeom = VertexGeom::Create(ds, "VertexGeom");
  auto* vertices = Float32Array::CreateWithStore<DataStore<float32>>(ds, "SharedVertexList", {numVertices}, {3}, vertexGeom->getId());
  vertexGeom->setVertices(*vertices);

  // Set vertex coordinates to map to the desired voxel indices
  for(usize i = 0; i < numVertices; i++)
  {
    setVertexCoordsForVoxelIndex(*vertices, i, voxelIndices[i]);
  }

  auto* vertexAM = AttributeMatrix::Create(ds, "VertexData", {numVertices}, vertexGeom->getId());
  vertexGeom->setVertexAttributeMatrix(*vertexAM);

  auto* faceArr = Float64Array::CreateWithStore<DataStore<float64>>(ds, "FaceAreas", {numVertices}, {1}, vertexAM->getId());
  for(usize i = 0; i < numVertices; i++)
  {
    (*faceArr)[i] = faceAreas[i];
  }

  auto* maskArr = BoolArray::CreateWithStore<DataStore<bool>>(ds, "Mask", {numVertices}, {1}, vertexAM->getId());
  maskArr->fill(true);
  if(!allMasked)
  {
    auto& maskStore = maskArr->getDataStoreRef();
    maskStore.setValue(maskedOutIndex, false);
  }

  // Image Geometry: 4x4x1
  auto* imageGeom = ImageGeom::Create(ds, "ImageGeom");
  imageGeom->setDimensions({4, 4, 1});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  // Cell data attribute matrix (required for getCellDataPath())
  auto* cellAM = AttributeMatrix::Create(ds, k_InterpolatedGroupName, {1, 4, 4}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  return ds;
}

Arguments getBaseArgs(bool useMask, uint64 technique, std::vector<float32> kernelSize, bool findLength = false, bool findMin = false, bool findMax = false, bool findMean = false,
                      bool findStdDev = false, bool findSum = false)
{
  using F = InterpolatePointCloudToRegularGridFilter;
  Arguments args;

  args.insertOrAssign(F::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(F::k_InterpolationTechnique_Key, std::make_any<uint64>(technique));
  args.insertOrAssign(F::k_KernelSize_Key, std::make_any<std::vector<float32>>(kernelSize));
  args.insertOrAssign(F::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1.0f, 1.0f, 1.0f}));
  args.insertOrAssign(F::k_SelectedVertexGeometryPath_Key, std::make_any<DataPath>(k_VertexGeomPath));
  args.insertOrAssign(F::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(F::k_InputMaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(F::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_FaceAreasPath}));
  args.insertOrAssign(F::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));

  args.insertOrAssign(F::k_FindLength_Key, std::make_any<bool>(findLength));
  args.insertOrAssign(F::k_FindMin_Key, std::make_any<bool>(findMin));
  args.insertOrAssign(F::k_FindMax_Key, std::make_any<bool>(findMax));
  args.insertOrAssign(F::k_FindMean_Key, std::make_any<bool>(findMean));
  args.insertOrAssign(F::k_FindStdDeviation_Key, std::make_any<bool>(findStdDev));
  args.insertOrAssign(F::k_FindSummation_Key, std::make_any<bool>(findSum));

  args.insertOrAssign(F::k_LengthSuffix_Key, std::make_any<std::string>("_Length"));
  args.insertOrAssign(F::k_MinSuffix_Key, std::make_any<std::string>("_Minimum"));
  args.insertOrAssign(F::k_MaxSuffix_Key, std::make_any<std::string>("_Maximum"));
  args.insertOrAssign(F::k_MeanSuffix_Key, std::make_any<std::string>("_Mean"));
  args.insertOrAssign(F::k_StdDeviationSuffix_Key, std::make_any<std::string>("_StdDeviation"));
  args.insertOrAssign(F::k_SummationSuffix_Key, std::make_any<std::string>("_Summation"));

  return args;
}
} // namespace

TEST_CASE("SimplnxCore::InterpolatePointCloudToRegularGridFilter: Uniform No Spreading With Statistics", "[SimplnxCore][InterpolatePointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  // 5 vertices: 2 map to voxel 0, 1 each to voxels 5, 10, 15
  // ImageGeom: 4x4x1 (16 voxels), kernel < spacing means no spreading
  std::vector<uint64> voxelIndices = {0, 0, 5, 10, 15};
  std::vector<float64> faceAreas = {10.0, 20.0, 30.0, 40.0, 50.0};

  DataStructure dataStructure = createTestDataStructure(voxelIndices, faceAreas);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args = getBaseArgs(false, InterpolatePointCloudToRegularGrid::k_Uniform, {0.5f, 0.5f, 0.5f}, true, true, true, true, true, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Interpolated FaceAreas (weighted average, float64)
  auto& interpFA = dataStructure.getDataRefAs<Float64Array>(k_InterpGroupPath.createChildPath("FaceAreas"));
  REQUIRE(interpFA[0] == Approx(15.0));  // (10+20)/2
  REQUIRE(interpFA[5] == Approx(30.0));  // 30/1
  REQUIRE(interpFA[10] == Approx(40.0)); // 40/1
  REQUIRE(interpFA[15] == Approx(50.0)); // 50/1
  REQUIRE(interpFA[1] == Approx(0.0));   // no contribution

  // Length (uint64)
  auto& length = dataStructure.getDataRefAs<UInt64Array>(k_InterpGroupPath.createChildPath("FaceAreas_Length"));
  REQUIRE(length[0] == 2);
  REQUIRE(length[5] == 1);
  REQUIRE(length[10] == 1);
  REQUIRE(length[15] == 1);
  REQUIRE(length[1] == 0);

  // Minimum (float32)
  auto& minArr = dataStructure.getDataRefAs<Float32Array>(k_InterpGroupPath.createChildPath("FaceAreas_Minimum"));
  REQUIRE(minArr[0] == Approx(10.0f));
  REQUIRE(minArr[5] == Approx(30.0f));

  // Maximum (float32)
  auto& maxArr = dataStructure.getDataRefAs<Float32Array>(k_InterpGroupPath.createChildPath("FaceAreas_Maximum"));
  REQUIRE(maxArr[0] == Approx(20.0f));
  REQUIRE(maxArr[5] == Approx(30.0f));

  // Mean (float32)
  auto& meanArr = dataStructure.getDataRefAs<Float32Array>(k_InterpGroupPath.createChildPath("FaceAreas_Mean"));
  REQUIRE(meanArr[0] == Approx(15.0f));
  REQUIRE(meanArr[5] == Approx(30.0f));

  // Standard Deviation (float32) - population stddev via Welford
  auto& stdArr = dataStructure.getDataRefAs<Float32Array>(k_InterpGroupPath.createChildPath("FaceAreas_StdDeviation"));
  REQUIRE(stdArr[0] == Approx(5.0f)); // sqrt(((10-15)^2 + (20-15)^2)/2) = 5
  REQUIRE(stdArr[5] == Approx(0.0f)); // single value

  // Summation (float32)
  auto& sumArr = dataStructure.getDataRefAs<Float32Array>(k_InterpGroupPath.createChildPath("FaceAreas_Summation"));
  REQUIRE(sumArr[0] == Approx(30.0f));  // 10+20
  REQUIRE(sumArr[5] == Approx(30.0f));  // 30
  REQUIRE(sumArr[10] == Approx(40.0f)); // 40
}

TEST_CASE("SimplnxCore::InterpolatePointCloudToRegularGridFilter: Uniform With Kernel Spreading", "[SimplnxCore][InterpolatePointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  // 1 vertex at voxel 5 (x=1,y=1,z=0) in a 4x4x1 grid
  // Kernel size 2.0, spacing 1.0 -> kernelNumVoxels = (1,1,1) -> 3x3x1 effective kernel
  // With Uniform kernel, all weights = 1.0
  // Vertex affects voxels: (0,0,0)=0, (1,0,0)=1, (2,0,0)=2,
  //                        (0,1,0)=4, (1,1,0)=5, (2,1,0)=6,
  //                        (0,2,0)=8, (1,2,0)=9, (2,2,0)=10
  std::vector<uint64> voxelIndices = {5};
  std::vector<float64> faceAreas = {10.0};

  DataStructure dataStructure = createTestDataStructure(voxelIndices, faceAreas);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args = getBaseArgs(false, InterpolatePointCloudToRegularGrid::k_Uniform, {2.0f, 2.0f, 2.0f}, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto& interpFA = dataStructure.getDataRefAs<Float64Array>(k_InterpGroupPath.createChildPath("FaceAreas"));
  auto& length = dataStructure.getDataRefAs<UInt64Array>(k_InterpGroupPath.createChildPath("FaceAreas_Length"));

  // All 9 affected voxels should have interp = 10.0 and length = 1
  std::vector<usize> affectedVoxels = {0, 1, 2, 4, 5, 6, 8, 9, 10};
  for(usize v : affectedVoxels)
  {
    REQUIRE(interpFA[v] == Approx(10.0));
    REQUIRE(length[v] == 1);
  }

  // Unaffected voxels should be 0
  std::vector<usize> unaffectedVoxels = {3, 7, 11, 12, 13, 14, 15};
  for(usize v : unaffectedVoxels)
  {
    REQUIRE(interpFA[v] == Approx(0.0));
    REQUIRE(length[v] == 0);
  }
}

TEST_CASE("SimplnxCore::InterpolatePointCloudToRegularGridFilter: Gaussian With Kernel Spreading", "[SimplnxCore][InterpolatePointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  // 2 vertices: voxel 5 (x=1,y=1,z=0) FaceArea=10.0, voxel 6 (x=2,y=1,z=0) FaceArea=20.0
  // Kernel size 2.0, spacing 1.0 -> 3x3x1 Gaussian kernel, sigmas=(1,1,1)
  std::vector<uint64> voxelIndices = {5, 6};
  std::vector<float64> faceAreas = {10.0, 20.0};

  DataStructure dataStructure = createTestDataStructure(voxelIndices, faceAreas);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args = getBaseArgs(false, InterpolatePointCloudToRegularGrid::k_Gaussian, {2.0f, 2.0f, 2.0f});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto& interpFA = dataStructure.getDataRefAs<Float64Array>(k_InterpGroupPath.createChildPath("FaceAreas"));

  // Voxel 5 (x=1,y=1): vertex0 at dx=0 w=1.0, vertex1 at dx=1 w=exp(-0.5)
  float64 w_center = 1.0;
  float64 w_adj = std::exp(-0.5);
  float64 expected5 = (w_center * 10.0 + w_adj * 20.0) / (w_center + w_adj);
  REQUIRE(interpFA[5] == Approx(expected5).epsilon(1e-6));

  // Voxel 6 (x=2,y=1): vertex0 at dx=-1 w=exp(-0.5), vertex1 at dx=0 w=1.0
  float64 expected6 = (w_adj * 10.0 + w_center * 20.0) / (w_adj + w_center);
  REQUIRE(interpFA[6] == Approx(expected6).epsilon(1e-6));

  // Voxel 5 should be < 15 (weighted toward 10.0 since vertex 0 is at center)
  // Voxel 6 should be > 15 (weighted toward 20.0 since vertex 1 is at center)
  REQUIRE(interpFA[5] < 15.0);
  REQUIRE(interpFA[6] > 15.0);
}

TEST_CASE("SimplnxCore::InterpolatePointCloudToRegularGridFilter: Masked Vertices", "[SimplnxCore][InterpolatePointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  // 5 vertices, mask out vertex 1 (second vertex at voxel 0 with FaceArea=20.0)
  std::vector<uint64> voxelIndices = {0, 0, 5, 10, 15};
  std::vector<float64> faceAreas = {10.0, 20.0, 30.0, 40.0, 50.0};

  DataStructure dataStructure = createTestDataStructure(voxelIndices, faceAreas, false, 1);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args = getBaseArgs(true, InterpolatePointCloudToRegularGrid::k_Uniform, {0.5f, 0.5f, 0.5f}, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto& interpFA = dataStructure.getDataRefAs<Float64Array>(k_InterpGroupPath.createChildPath("FaceAreas"));
  auto& length = dataStructure.getDataRefAs<UInt64Array>(k_InterpGroupPath.createChildPath("FaceAreas_Length"));

  // Voxel 0: only vertex 0 (vertex 1 masked out)
  REQUIRE(interpFA[0] == Approx(10.0));
  REQUIRE(length[0] == 1);

  // Others unchanged
  REQUIRE(interpFA[5] == Approx(30.0));
  REQUIRE(interpFA[10] == Approx(40.0));
  REQUIRE(interpFA[15] == Approx(50.0));
}

TEST_CASE("SimplnxCore::InterpolatePointCloudToRegularGridFilter: Invalid Filter Execution", "[SimplnxCore][InterpolatePointCloudToRegularGridFilter]")
{
  UnitTest::LoadPlugins();

  std::vector<uint64> voxelIndices = {0, 5};
  std::vector<float64> faceAreas = {10.0, 20.0};
  DataStructure dataStructure = createTestDataStructure(voxelIndices, faceAreas);

  InterpolatePointCloudToRegularGridFilter filter;

  SECTION("Invalid Kernel Size")
  {
    Arguments args = getBaseArgs(false, InterpolatePointCloudToRegularGrid::k_Gaussian, {-1.0f, 1.0f, 1.0f});
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  SECTION("Invalid Gaussian Sigma")
  {
    Arguments args = getBaseArgs(false, InterpolatePointCloudToRegularGrid::k_Gaussian, {1.0f, 1.0f, 1.0f});
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0.0f, 0.0f, 0.0f}));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
}

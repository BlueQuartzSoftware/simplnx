#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_UniformInterpolatedData = "UniformInterpolatedData";
const std::string k_GaussianInterpolatedData = "GaussianInterpolatedData";
const std::string k_Computed = "[Computed]";
const std::string k_KernalDistances = "KernelDistances";

const DataPath k_ImageGeomPath({k_ImageGeometry});
const DataPath k_VertexGeometryPath({k_PointCloudContainerName});
const DataPath k_VertexDataPath = k_VertexGeometryPath.createChildPath(k_VertexData);
const DataPath k_MaskPath = k_VertexDataPath.createChildPath(k_Mask);
const DataPath k_FaceAreasPath = k_VertexDataPath.createChildPath(k_FaceAreas);
const DataPath k_VoxelIndicesPath = k_VertexDataPath.createChildPath(k_VoxelIndices);

const DataPath k_UniformInterpolatedDataExemplar = k_ImageGeomPath.createChildPath(k_UniformInterpolatedData);
const DataPath k_UniformInterpolatedDataComputed = k_ImageGeomPath.createChildPath(k_UniformInterpolatedData + k_Computed);
const DataPath k_GaussianInterpolatedDataExemplar = k_ImageGeomPath.createChildPath(k_GaussianInterpolatedData);
const DataPath k_GaussianInterpolatedDataComputed = k_ImageGeomPath.createChildPath(k_GaussianInterpolatedData + k_Computed);

const DataPath k_UniformFaceAreasExemplar = k_UniformInterpolatedDataExemplar.createChildPath(k_FaceAreas);
const DataPath k_UniformVoxelIndicesExemplar = k_UniformInterpolatedDataExemplar.createChildPath(k_VoxelIndices);
const DataPath k_UniformKernalDistancesExemplar = k_UniformInterpolatedDataExemplar.createChildPath(k_KernalDistances);
const DataPath k_UniformFaceAreasComputed = k_UniformInterpolatedDataComputed.createChildPath(k_FaceAreas);
const DataPath k_UniformVoxelIndicesComputed = k_UniformInterpolatedDataComputed.createChildPath(k_VoxelIndices);
const DataPath k_UniformKernalDistancesComputed = k_UniformInterpolatedDataComputed.createChildPath(k_KernalDistances);

const DataPath k_GaussianFaceAreasExemplar = k_GaussianInterpolatedDataExemplar.createChildPath(k_FaceAreas);
const DataPath k_GaussianVoxelIndicesExemplar = k_GaussianInterpolatedDataExemplar.createChildPath(k_VoxelIndices);
const DataPath k_GaussianKernalDistancesExemplar = k_GaussianInterpolatedDataExemplar.createChildPath(k_KernalDistances);
const DataPath k_GaussianFaceAreasComputed = k_GaussianInterpolatedDataComputed.createChildPath(k_FaceAreas);
const DataPath k_GaussianVoxelIndicesComputed = k_GaussianInterpolatedDataComputed.createChildPath(k_VoxelIndices);
const DataPath k_GaussianKernalDistancesComputed = k_GaussianInterpolatedDataComputed.createChildPath(k_KernalDistances);
} // namespace

TEST_CASE("ComplexCore::InterpolatePointCloudToRegularGridFilter: Valid Filter Execution - Uniform Inpterpolation with Mask", "[ComplexCore][InterpolatePointCloudToRegularGridFilter]")
{
  const std::string kDataInputArchive = "6_6_interpolate_point_cloud_to_regular_grid.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_interpolate_point_cloud_to_regular_grid";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_interpolate_point_cloud_to_regular_grid/6_6_interpolate_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args;

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(true));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(InterpolatePointCloudToRegularGridFilter::k_Uniform));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(k_VoxelIndicesPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_Mask_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_FaceAreasPath}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_VoxelIndicesPath}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<std::string>(k_UniformInterpolatedDataComputed.getTargetName()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesArray_Key, std::make_any<std::string>(k_UniformKernalDistancesComputed.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareNeighborLists<float64>(dataStructure, k_UniformFaceAreasExemplar, k_UniformFaceAreasComputed);
  UnitTest::CompareNeighborLists<uint64>(dataStructure, k_UniformVoxelIndicesExemplar, k_UniformVoxelIndicesComputed);
  UnitTest::CompareNeighborLists<float32>(dataStructure, k_UniformKernalDistancesExemplar, k_UniformKernalDistancesComputed);
}

TEST_CASE("ComplexCore::InterpolatePointCloudToRegularGridFilter: Valid Filter Execution - Gaussian Inpterpolation", "[ComplexCore][InterpolatePointCloudToRegularGridFilter]")
{
  const std::string kDataInputArchive = "6_6_interpolate_point_cloud_to_regular_grid.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_interpolate_point_cloud_to_regular_grid";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_interpolate_point_cloud_to_regular_grid/6_6_interpolate_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args;

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(true));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(InterpolatePointCloudToRegularGridFilter::k_Gaussian));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(k_VoxelIndicesPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_FaceAreasPath}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_VoxelIndicesPath}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<std::string>(k_GaussianInterpolatedDataComputed.getTargetName()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesArray_Key, std::make_any<std::string>(k_GaussianKernalDistancesComputed.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareNeighborLists<float64>(dataStructure, k_GaussianFaceAreasExemplar, k_GaussianFaceAreasComputed);
  UnitTest::CompareNeighborLists<uint64>(dataStructure, k_GaussianVoxelIndicesExemplar, k_GaussianVoxelIndicesComputed);
  UnitTest::CompareNeighborLists<float32>(dataStructure, k_GaussianKernalDistancesExemplar, k_GaussianKernalDistancesComputed);
}

TEST_CASE("ComplexCore::InterpolatePointCloudToRegularGridFilter: Invalid Filter Execution", "[ComplexCore][InterpolatePointCloudToRegularGridFilter]")
{
  const std::string kDataInputArchive = "6_6_interpolate_point_cloud_to_regular_grid.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_interpolate_point_cloud_to_regular_grid";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_interpolate_point_cloud_to_regular_grid/6_6_interpolate_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  InterpolatePointCloudToRegularGridFilter filter;
  Arguments args;

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(true));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(InterpolatePointCloudToRegularGridFilter::k_Gaussian));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(k_VoxelIndicesPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_VoxelIndicesPath}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<std::string>(k_GaussianInterpolatedDataComputed.getTargetName()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesArray_Key, std::make_any<std::string>(k_GaussianKernalDistancesComputed.getTargetName()));

  SECTION("Invalid Kernel Size")
  {
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{-1, 1, 1}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_FaceAreasPath}));
  }
  SECTION("Invalid Gaussian Sigma")
  {
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_FaceAreasPath}));
  }
  SECTION("Mismatching Input Array Tuples")
  {
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{k_GaussianFaceAreasExemplar}));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}

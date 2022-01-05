#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/InterpolatePointCloudToRegularGridFilter.hpp"

#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("InterpolatePointCloudToRegularGridFilter: Create Filter", "[DREAM3DReview][InterpolatePointCloudToRegularGridFilter][.][UNIMPLEMENTED][!mayfail]")
{
  InterpolatePointCloudToRegularGridFilter filter;
  DataStructure dataGraph;
  Arguments args;

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(false));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(1));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_Mask_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<DataPath>(DataPath()));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesGroup_Key, std::make_any<DataPath>(DataPath()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("InterpolatePointCloudToRegularGridFilter: Test Algorithm 1", "[DREAM3DReview][InterpolatePointCloudToRegularGridFilter][.][UNIMPLEMENTED][!mayfail]")
{
  InterpolatePointCloudToRegularGridFilter filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  bool useMask = true;
  bool storeKernelDistances = true;
  uint64 interpolationTechnique = 0;
  std::vector<float32> kernelSize = {1, 1, 1};
  std::vector<float32> gaussianSigmas = {1, 1, 1};
  DataPath vertexPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath imagePath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  DataPath voxelIndices = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Voxel Indices"});
  DataPath maskPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "ConditionalArray"});
  std::vector<DataPath> interpolateArrays = {DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "ConditionalArray"})};
  std::vector<DataPath> copyArrays = {DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"})};
  DataPath interpolatedGroupPath = DataPath({Constants::k_EbsdScanData, "Interpolated Group"});
  DataPath kernelDistancesGroupPath = DataPath({Constants::k_EbsdScanData, "Kernel Distances"});

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(storeKernelDistances));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(interpolationTechnique));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(kernelSize));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(gaussianSigmas));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(vertexPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(imagePath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(voxelIndices));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_Mask_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(interpolateArrays));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(copyArrays));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<DataPath>(interpolatedGroupPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesGroup_Key, std::make_any<DataPath>(kernelDistancesGroupPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}

TEST_CASE("InterpolatePointCloudToRegularGridFilter: Test Algorithm 2", "[DREAM3DReview][InterpolatePointCloudToRegularGridFilter]")
{
  InterpolatePointCloudToRegularGridFilter filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  bool useMask = true;
  bool storeKernelDistances = true;
  uint64 interpolationTechnique = 1;
  std::vector<float32> kernelSize = {1, 1, 1};
  std::vector<float32> gaussianSigmas = {1, 1, 1};
  DataPath vertexPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath imagePath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ImageGeometry});
  DataPath voxelIndices = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Voxel Indices"});
  DataPath maskPath = DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "ConditionalArray"});
  std::vector<DataPath> interpolateArrays = {DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "ConditionalArray"})};
  std::vector<DataPath> copyArrays = {DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"})};
  DataPath interpolatedGroupPath = DataPath({Constants::k_EbsdScanData, "Interpolated Group"});
  DataPath kernelDistancesGroupPath = DataPath({Constants::k_EbsdScanData, "Kernel Distances"});

  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_StoreKernelDistances_Key, std::make_any<bool>(storeKernelDistances));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolationTechnique_Key, std::make_any<uint64>(interpolationTechnique));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelSize_Key, std::make_any<std::vector<float32>>(kernelSize));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_GaussianSigmas_Key, std::make_any<std::vector<float32>>(gaussianSigmas));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VertexGeom_Key, std::make_any<DataPath>(vertexPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_ImageGeom_Key, std::make_any<DataPath>(imagePath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(voxelIndices));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_Mask_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolateArrays_Key, std::make_any<std::vector<DataPath>>(interpolateArrays));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_CopyArrays_Key, std::make_any<std::vector<DataPath>>(copyArrays));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_InterpolatedGroup_Key, std::make_any<DataPath>(interpolatedGroupPath));
  args.insertOrAssign(InterpolatePointCloudToRegularGridFilter::k_KernelDistancesGroup_Key, std::make_any<DataPath>(kernelDistancesGroupPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}

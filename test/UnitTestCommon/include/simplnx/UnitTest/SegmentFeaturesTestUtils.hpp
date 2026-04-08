#pragma once

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <array>
#include <cmath>
#include <set>
#include <unordered_map>
#include <vector>

namespace nx::core::UnitTest
{

/**
 * @brief Creates an ImageGeom with a CellData AttributeMatrix.
 * @param ds DataStructure to create objects in.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param geomName Name for the ImageGeom.
 * @param cellDataName Name for the CellData AttributeMatrix.
 * @return Pointer to the created AttributeMatrix.
 */
inline AttributeMatrix* BuildSegmentFeaturesTestGeometry(DataStructure& ds, const std::array<usize, 3>& dims, const std::string& geomName, const std::string& cellDataName)
{
  auto* geom = ImageGeom::Create(ds, geomName);
  geom->setDimensions({dims[0], dims[1], dims[2]});
  geom->setSpacing({1.0f, 1.0f, 1.0f});
  geom->setOrigin({0.0f, 0.0f, 0.0f});

  const ShapeType cellShape = {dims[2], dims[1], dims[0]};
  auto* am = AttributeMatrix::Create(ds, cellDataName, cellShape, geom->getId());
  geom->setCellData(*am);
  return am;
}

/**
 * @brief Creates block-patterned int32 scalar data for ScalarSegmentFeatures testing.
 * @param ds DataStructure to create the array in.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param amId Parent AttributeMatrix ID.
 * @param blockSize Voxel count per block edge.
 * @param arrayName Name for the scalar array.
 */
inline void BuildScalarTestData(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType amId, usize blockSize, const std::string& arrayName = "ScalarData", bool wrapBoundary = false)
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  auto scalarDataStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* scalarArray = DataArray<int32>::Create(ds, arrayName, scalarDataStore, amId);
  auto& store = scalarArray->getDataStoreRef();

  const usize blocksPerX = (dimX + blockSize - 1) / blockSize;
  const usize blocksPerY = (dimY + blockSize - 1) / blockSize;
  const usize blocksPerZ = (dimZ + blockSize - 1) / blockSize;

  const usize sliceSize = dimY * dimX;
  std::vector<int32> sliceBuffer(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize bx = x / blockSize;
        const usize by = y / blockSize;
        const usize bz = z / blockSize;

        if(wrapBoundary)
        {
          // Last block in each axis maps to the same value as the first block,
          // so periodic wrapping merges them into one feature.
          const usize wbx = (bx == blocksPerX - 1) ? 0 : bx;
          const usize wby = (by == blocksPerY - 1) ? 0 : by;
          const usize wbz = (bz == blocksPerZ - 1) ? 0 : bz;
          const usize wbpx = blocksPerX - 1;
          const usize wbpy = blocksPerY - 1;
          sliceBuffer[y * dimX + x] = static_cast<int32>(wbz * wbpy * wbpx + wby * wbpx + wbx);
        }
        else
        {
          sliceBuffer[y * dimX + x] = static_cast<int32>(bz * blocksPerY * blocksPerX + by * blocksPerX + bx);
        }
      }
    }
    store.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(sliceBuffer.data(), sliceSize));
  }
}

/**
 * @brief Creates quaternion, phase, and crystal structure arrays for EBSD/CAxis testing.
 *
 * Quaternions are block-patterned with distinct orientations per block.
 * All voxels are assigned phase 1. CrystalStructures has phase 0 = 999 (Unknown)
 * and phase 1 = the provided crystal structure value.
 *
 * @param ds DataStructure to create arrays in.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param geomId Parent geometry ID (for ensemble AM).
 * @param amId Parent CellData AttributeMatrix ID.
 * @param crystalStructure Crystal structure for phase 1 (1 = Cubic_High, 0 = Hexagonal_High).
 * @param blockSize Voxel count per block edge.
 */
inline void BuildOrientationTestData(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType geomId, DataObject::IdType amId, uint32 crystalStructure, usize blockSize,
                                     bool wrapBoundary = false)
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  auto quatsDataStore = DataStoreUtilities::CreateDataStore<float32>(cellShape, {4}, IDataAction::Mode::Execute);
  auto* quatsArray = DataArray<float32>::Create(ds, "Quats", quatsDataStore, amId);
  auto& quatsStore = quatsArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(ds, "Phases", phasesDataStore, amId);
  auto& phasesStore = phasesArray->getDataStoreRef();

  constexpr float32 k_DegToRad = 3.14159265358979323846f / 180.0f;

  const usize blocksPerX = (dimX + blockSize - 1) / blockSize;
  const usize blocksPerY = (dimY + blockSize - 1) / blockSize;
  const usize blocksPerZ = (dimZ + blockSize - 1) / blockSize;
  const usize numBlocks = blocksPerX * blocksPerY * blocksPerZ;

  // Quaternion Hamilton product: result = a * b, where q = (w, x, y, z)
  auto quatMul = [](const std::array<float32, 4>& a, const std::array<float32, 4>& b) -> std::array<float32, 4> {
    return {a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3], a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2], a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
            a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]};
  };

  std::vector<std::array<float32, 4>> blockQuats(numBlocks);

  // Z-layer orientation scheme (shared by EBSD and CAxis):
  // All blocks in the same Z-layer share a single X-axis rotation angle.
  // This produces 3 horizontal layers of identical orientations:
  //   z=0: 0° rotation   → q = [1, 0, 0, 0]      c-axis = [0, 0, 1]
  //   z=1: 30° rotation  → q = [0.966, 0.259, 0, 0] c-axis = [0, 0.5, 0.866]
  //   z=2: 60° rotation  → q = [0.866, 0.5, 0, 0]   c-axis = [0, 0.866, 0.5]
  //
  // Adjacent layers differ by 30°, well above the 5° tolerance → no merge.
  // Within each layer, all blocks share the same angle → they merge.
  //
  // Merge pair override (non-periodic only):
  //   Block (1,1,1) at center of z=1 is set to 0° instead of 30°.
  //   It merges with its z=0 neighbor (1,1,0) while staying separate
  //   from the other z=1 blocks (30° difference → no merge).
  //
  // Expected features (3x3x3 grid):
  //   Base (3 blocks/axis): 3 features (z=0 + center pillar, z=1 minus pillar, z=2)
  //   Base (8 blocks/axis): 3 features (repeating 0°/30°/60° stripes)
  //   Periodic: layers sharing the same angle merge across the boundary
  constexpr float32 k_LayerAngles[] = {0.0f, 30.0f, 60.0f};

  for(usize bz = 0; bz < blocksPerZ; bz++)
  {
    const usize layerIdx = bz % 3;
    const float32 halfAngle = k_LayerAngles[layerIdx] * k_DegToRad * 0.5f;
    // EBSDlib quaternion layout: (x, y, z, w) — Vector-Scalar order
    const std::array<float32, 4> layerQuat = {std::sin(halfAngle), 0.0f, 0.0f, std::cos(halfAngle)};

    for(usize by = 0; by < blocksPerY; by++)
    {
      for(usize bx = 0; bx < blocksPerX; bx++)
      {
        const usize blockIdx = bz * blocksPerY * blocksPerX + by * blocksPerX + bx;
        blockQuats[blockIdx] = layerQuat;
      }
    }
  }

  // Merge pair: block (1,1,1) gets z=0 angle (0°) instead of z=1 angle (30°).
  // It merges downward into the z=0 layer through face neighbor (1,1,0).
  if(!wrapBoundary && blocksPerX >= 3 && blocksPerY >= 3 && blocksPerZ >= 3)
  {
    const usize idx_111 = 1 * blocksPerY * blocksPerX + 1 * blocksPerX + 1;
    blockQuats[idx_111] = blockQuats[0]; // Set to 0° (z=0 layer angle)
  }

  const usize sliceSize = dimY * dimX;
  std::vector<float32> quatsSliceBuffer(sliceSize * 4);
  std::vector<int32> phasesSliceBuffer(sliceSize, 1);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        if(wrapBoundary)
        {
          bx = (bx == blocksPerX - 1) ? 0 : bx;
          by = (by == blocksPerY - 1) ? 0 : by;
          bz = (bz == blocksPerZ - 1) ? 0 : bz;
        }
        const usize blockIdx = bz * blocksPerY * blocksPerX + by * blocksPerX + bx;
        const auto& q = blockQuats[blockIdx];
        const usize bufIdx = (y * dimX + x) * 4;
        quatsSliceBuffer[bufIdx + 0] = q[0];
        quatsSliceBuffer[bufIdx + 1] = q[1];
        quatsSliceBuffer[bufIdx + 2] = q[2];
        quatsSliceBuffer[bufIdx + 3] = q[3];
      }
    }
    quatsStore.copyFromBuffer(z * sliceSize * 4, nonstd::span<const float32>(quatsSliceBuffer.data(), sliceSize * 4));
    phasesStore.copyFromBuffer(z * sliceSize, nonstd::span<const int32>(phasesSliceBuffer.data(), sliceSize));
  }

  // Create CellEnsembleData with CrystalStructures
  const ShapeType ensembleTupleShape = {2};
  auto* ensembleAM = AttributeMatrix::Create(ds, "CellEnsembleData", ensembleTupleShape, geomId);
  auto crystalDataStore = DataStoreUtilities::CreateDataStore<uint32>(ensembleTupleShape, {1}, IDataAction::Mode::Execute);
  auto* crystalStructsArray = DataArray<uint32>::Create(ds, "CrystalStructures", crystalDataStore, ensembleAM->getId());
  auto& crystalStructsStore = crystalStructsArray->getDataStoreRef();
  crystalStructsStore[0] = 999; // Phase 0: Unknown
  crystalStructsStore[1] = crystalStructure;
}

/**
 * @brief Creates a spherical mask array where voxels inside the sphere are 1 (good)
 * and voxels outside are 0 (masked out).
 *
 * The sphere is centered in the volume with radius = 80% of half the smallest dimension.
 * For a 200x200x200 volume, that gives a radius of 80 voxels.
 *
 * @param ds DataStructure to create the array in.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param amId Parent AttributeMatrix ID.
 * @param maskName Name for the mask array.
 */
inline void BuildSphericalMask(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType amId, const std::string& maskName = "Mask")
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  auto maskDataStore = DataStoreUtilities::CreateDataStore<uint8>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* maskArray = DataArray<uint8>::Create(ds, maskName, maskDataStore, amId);
  auto& maskStore = maskArray->getDataStoreRef();

  const float32 cx = static_cast<float32>(dimX) / 2.0f;
  const float32 cy = static_cast<float32>(dimY) / 2.0f;
  const float32 cz = static_cast<float32>(dimZ) / 2.0f;
  const float32 radius = std::min({cx, cy, cz}) * 0.8f;

  const usize sliceSize = dimY * dimX;
  std::vector<uint8> sliceBuffer(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const float32 dx = static_cast<float32>(x) - cx;
        const float32 dy = static_cast<float32>(y) - cy;
        const float32 dz = static_cast<float32>(z) - cz;
        sliceBuffer[y * dimX + x] = (dx * dx + dy * dy + dz * dz < radius * radius) ? 1 : 0;
      }
    }
    maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const uint8>(sliceBuffer.data(), sliceSize));
  }
}

/**
 * @brief Verifies segmentation results when a mask is applied.
 *
 * Checks that:
 * 1. Masked voxels (mask=0) have FeatureId=0
 * 2. Unmasked voxels (mask=1) have FeatureId > 0
 * 3. At least one feature was created
 * 4. Both masked and unmasked regions exist
 *
 * @param ds DataStructure containing the results.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param featureIdsPath Path to the generated FeatureIds array.
 * @param activePath Path to the generated Active array.
 * @param maskPath Path to the mask array.
 */
inline void VerifyMaskedSegmentation(const DataStructure& ds, const std::array<usize, 3>& dims, const DataPath& featureIdsPath, const DataPath& activePath, const DataPath& maskPath)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(featureIdsPath));
  const auto& featureIds = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();

  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(maskPath));
  const auto& mask = ds.getDataRefAs<UInt8Array>(maskPath);
  const auto& maskStore = mask.getDataStoreRef();

  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(activePath));
  const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
  REQUIRE(actives.getNumberOfTuples() > 1); // At least one feature (index 0 + features)

  const usize totalVoxels = dims[0] * dims[1] * dims[2];
  usize maskedCount = 0;
  usize unmaskedCount = 0;

  for(usize i = 0; i < totalVoxels; i++)
  {
    if(maskStore.getValue(i) == 0)
    {
      REQUIRE(featureStore.getValue(i) == 0);
      maskedCount++;
    }
    else
    {
      REQUIRE(featureStore.getValue(i) > 0);
      unmaskedCount++;
    }
  }

  REQUIRE(maskedCount > 0);
  REQUIRE(unmaskedCount > 0);
}

/**
 * @brief Verifies that block-patterned segmentation produced the expected results.
 *
 * Checks that:
 * 1. The feature count matches the expected number of blocks
 * 2. All voxels within a block share the same FeatureId
 * 3. Different blocks have different FeatureIds
 *
 * @param ds DataStructure containing the results.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param blockSize Voxel count per block edge.
 * @param featureIdsPath Path to the generated FeatureIds array.
 * @param activePath Path to the generated Active array.
 */
inline void VerifyBlockSegmentation(const DataStructure& ds, const std::array<usize, 3>& dims, usize blockSize, const DataPath& featureIdsPath, const DataPath& activePath)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize blocksPerX = (dimX + blockSize - 1) / blockSize;
  const usize blocksPerY = (dimY + blockSize - 1) / blockSize;
  const usize blocksPerZ = (dimZ + blockSize - 1) / blockSize;
  const usize expectedFeatures = blocksPerX * blocksPerY * blocksPerZ;

  // Check feature count (Active array includes Feature 0)
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(activePath));
  const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
  REQUIRE(actives.getNumberOfTuples() == expectedFeatures + 1);

  // Check FeatureIds consistency
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(featureIdsPath));
  const auto& featureIds = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();

  // Map from block index to the FeatureId assigned to that block
  std::unordered_map<usize, int32> blockToFeature;
  // Track all assigned FeatureIds to verify uniqueness
  std::set<int32> usedFeatureIds;

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize voxelIdx = z * dimX * dimY + y * dimX + x;
        const usize blockIdx = (z / blockSize) * blocksPerY * blocksPerX + (y / blockSize) * blocksPerX + (x / blockSize);
        const int32 featureId = featureStore.getValue(voxelIdx);

        REQUIRE(featureId > 0); // No voxel should be unassigned

        auto it = blockToFeature.find(blockIdx);
        if(it == blockToFeature.end())
        {
          blockToFeature[blockIdx] = featureId;
          usedFeatureIds.insert(featureId);
        }
        else
        {
          REQUIRE(it->second == featureId); // All voxels in a block share the same FeatureId
        }
      }
    }
  }

  // Each block should have a unique FeatureId
  REQUIRE(usedFeatureIds.size() == expectedFeatures);
}

/**
 * @brief Verifies segmentation results when periodic BCs are enabled and boundary
 * blocks have matching data (wrapBoundary=true).
 *
 * With periodic wrapping, the last block in each axis merges with the first block.
 * Expected feature count: (blocksPerX-1) * (blocksPerY-1) * (blocksPerZ-1).
 *
 * @param ds DataStructure containing the results.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param blockSize Voxel count per block edge.
 * @param featureIdsPath Path to the generated FeatureIds array.
 * @param activePath Path to the generated Active array.
 */
inline void VerifyPeriodicBlockSegmentation(const DataStructure& ds, const std::array<usize, 3>& dims, usize blockSize, const DataPath& featureIdsPath, const DataPath& activePath)
{
  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize dimZ = dims[2];
  const usize blocksPerX = (dimX + blockSize - 1) / blockSize;
  const usize blocksPerY = (dimY + blockSize - 1) / blockSize;
  const usize blocksPerZ = (dimZ + blockSize - 1) / blockSize;
  const usize periodicBlocksX = blocksPerX - 1;
  const usize periodicBlocksY = blocksPerY - 1;
  const usize periodicBlocksZ = blocksPerZ - 1;
  const usize expectedFeatures = periodicBlocksX * periodicBlocksY * periodicBlocksZ;

  // Check feature count (Active array includes Feature 0)
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(activePath));
  const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
  REQUIRE(actives.getNumberOfTuples() == expectedFeatures + 1);

  // Check FeatureIds consistency
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(featureIdsPath));
  const auto& featureIds = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();

  // Map from periodic block index to the FeatureId assigned to that block
  std::unordered_map<usize, int32> blockToFeature;
  std::set<int32> usedFeatureIds;

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize voxelIdx = z * dimX * dimY + y * dimX + x;
        const usize bx = x / blockSize;
        const usize by = y / blockSize;
        const usize bz = z / blockSize;

        // Effective periodic block index: last block wraps to first
        const usize pbx = bx % periodicBlocksX;
        const usize pby = by % periodicBlocksY;
        const usize pbz = bz % periodicBlocksZ;
        const usize periodicBlockIdx = pbz * periodicBlocksY * periodicBlocksX + pby * periodicBlocksX + pbx;

        const int32 featureId = featureStore.getValue(voxelIdx);
        REQUIRE(featureId > 0); // No voxel should be unassigned

        auto it = blockToFeature.find(periodicBlockIdx);
        if(it == blockToFeature.end())
        {
          blockToFeature[periodicBlockIdx] = featureId;
          usedFeatureIds.insert(featureId);
        }
        else
        {
          REQUIRE(it->second == featureId); // All voxels in matching periodic blocks share the same FeatureId
        }
      }
    }
  }

  // Each periodic block group should have a unique FeatureId
  REQUIRE(usedFeatureIds.size() == expectedFeatures);
}

/**
 * @brief Runs the "no valid voxels returns error -87000" test for any SegmentFeatures filter.
 *
 * Creates a 3x3x3 grid with all voxels masked out, runs the filter, and asserts
 * that execution returns error -87000.
 *
 * @tparam FilterT The filter class (e.g., ScalarSegmentFeaturesFilter).
 * @param setupArgs Lambda that receives (Arguments&, DataPath geomPath, DataPath cellDataPath, DataPath maskPath)
 *                  and inserts filter-specific arguments.
 */
template <typename FilterT, typename SetupArgsFn>
void RunNoValidVoxelsErrorTest(SetupArgsFn setupArgs)
{
  constexpr usize kDim = 3;
  const std::array<usize, 3> dims = {kDim, kDim, kDim};
  const ShapeType cellShape = {kDim, kDim, kDim};

  DataStructure ds;
  auto* am = BuildSegmentFeaturesTestGeometry(ds, dims, "Geom", "CellData");

  auto* mask = CreateTestDataArray<uint8>(ds, "Mask", cellShape, {1}, am->getId());
  mask->fill(0);

  const DataPath geomPath({"Geom"});
  const DataPath cellDataPath({"Geom", "CellData"});
  const DataPath maskPath({"Geom", "CellData", "Mask"});

  FilterT filter;
  Arguments args;
  setupArgs(args, ds, geomPath, cellDataPath, maskPath);

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  const auto& errors = executeResult.result.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == -87000);
}

/**
 * @brief Tests that FaceEdgeVertex (26-neighbor) connectivity correctly merges
 * regions connected through shared vertices and edges, not just faces.
 *
 * Creates a 3x3x3 geometry with 4 isolated single-voxel regions:
 *   - Regions A,B (same data): voxels (0,0,0) and (1,1,1) — vertex-connected only
 *   - Regions C,D (same data): voxels (2,0,0) and (2,1,1) — edge-connected only
 *
 * With Face (6-neighbor): 5 features (1 background + 4 isolated regions)
 * With FaceEdgeVertex (26-neighbor): 3 features (1 background + A&B merged + C&D merged)
 *
 * @tparam FilterT The filter class (e.g., ScalarSegmentFeaturesFilter).
 * @param setupFaceArgs Lambda (Arguments&, DataStructure&, DataPath geomPath, DataPath cellDataPath)
 *        that inserts filter-specific arguments with neighborScheme=0 (Face).
 * @param setupFevArgs Lambda with same signature but neighborScheme=1 (FaceEdgeVertex).
 */
template <typename FilterT, typename SetupFaceFn, typename SetupFevFn>
void RunFaceEdgeVertexConnectivityTest(SetupFaceFn setupFaceArgs, SetupFevFn setupFevArgs)
{
  constexpr usize kDim = 3;
  const std::array<usize, 3> dims = {kDim, kDim, kDim};
  const ShapeType cellShape = {kDim, kDim, kDim};

  const DataPath geomPath({"Geom"});
  const DataPath cellDataPath({"Geom", "CellData"});
  const DataPath featureIdsPath({"Geom", "CellData", "FeatureIds"});
  const DataPath activePath({"Geom", "CellFeatureData", "Active"});

  // Face scheme: A, B, C, D are all isolated → 5 features + index 0
  {
    DataStructure ds;
    BuildSegmentFeaturesTestGeometry(ds, dims, "Geom", "CellData");
    FilterT filter;
    Arguments args;
    setupFaceArgs(args, ds, geomPath, cellDataPath);
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
    REQUIRE(actives.getNumberOfTuples() == 6);
  }

  // FaceEdgeVertex scheme: A+B merge (vertex), C+D merge (edge) → 3 features + index 0
  DataStructure ds;
  BuildSegmentFeaturesTestGeometry(ds, dims, "Geom", "CellData");
  {
    FilterT filter;
    Arguments args;
    setupFevArgs(args, ds, geomPath, cellDataPath);
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
    REQUIRE(actives.getNumberOfTuples() == 4);
  }

  // Verify the vertex-connected pair shares a FeatureId
  const auto& fids = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& fidsStore = fids.getDataStoreRef();
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 0) == fidsStore.getValue(1 * 9 + 1 * 3 + 1)); // A == B (vertex merge)
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 2) == fidsStore.getValue(1 * 9 + 1 * 3 + 2)); // C == D (edge merge)
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 0) != fidsStore.getValue(0 * 9 + 0 * 3 + 2)); // A != C (different values)
}

/**
 * @brief Runs a SegmentFeatures filter against exemplar data and verifies results.
 *
 * Executes the filter, optionally checks the feature count, compares computed
 * FeatureIds against embedded exemplar arrays, and validates tuple dimension
 * inheritance. Used by Scalar, EBSD, and CAxis neighbor scheme tests.
 *
 * @tparam FilterT The filter class (e.g., ScalarSegmentFeaturesFilter).
 * @tparam SetupArgsFn Lambda (Arguments&) that inserts all filter-specific arguments.
 * @param dataStructure DataStructure loaded from an exemplar .dream3d file.
 * @param computedFeatureIdsPath Path where the filter writes its FeatureIds array.
 * @param activesPath Path where the filter writes its Active array.
 * @param exemplarFeatureIdsPath Path to the pre-computed exemplar FeatureIds.
 * @param expectedFeatureCount Expected Active tuple count (0 to skip this check).
 * @param setupArgs Lambda to populate filter Arguments.
 * @param tupleCheckIgnoredPaths Paths to exclude from CheckArraysInheritTupleDims.
 */
template <typename FilterT, typename SetupArgsFn>
void RunNeighborSchemeExemplarTest(DataStructure& dataStructure, const DataPath& computedFeatureIdsPath, const DataPath& activesPath, const DataPath& exemplarFeatureIdsPath,
                                   usize expectedFeatureCount, SetupArgsFn setupArgs, const std::vector<DataPath>& tupleCheckIgnoredPaths = {})
{
  FilterT filter;
  Arguments args;
  setupArgs(args);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  if(expectedFeatureCount > 0)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(activesPath));
    const auto& actives = dataStructure.getDataRefAs<UInt8Array>(activesPath);
    REQUIRE(actives.getNumberOfTuples() == expectedFeatureCount);
  }

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(computedFeatureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(exemplarFeatureIdsPath));
  const auto& generatedArray = dataStructure.getDataRefAs<Int32Array>(computedFeatureIdsPath);
  const auto& exemplarArray = dataStructure.getDataRefAs<Int32Array>(exemplarFeatureIdsPath);
  CompareDataArrays<int32>(generatedArray, exemplarArray);

  CheckArraysInheritTupleDims(dataStructure, tupleCheckIgnoredPaths);
}

} // namespace nx::core::UnitTest

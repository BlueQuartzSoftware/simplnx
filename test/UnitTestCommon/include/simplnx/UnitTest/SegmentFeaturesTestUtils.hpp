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
 * @brief Creates an ImageGeom and its CellData AttributeMatrix.
 * @param ds Receives the created objects.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param geomName ImageGeom name.
 * @param cellDataName CellData AttributeMatrix name.
 * @return The created CellData AttributeMatrix.
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
 * @brief Creates block-patterned int32 scalar data for ScalarSegmentFeatures tests.
 * @param ds Receives the scalar array.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param amId Parent AttributeMatrix identifier.
 * @param blockSize Number of voxels on each block edge.
 * @param arrayName Scalar array name.
 * @param wrapBoundary True to give opposite boundary blocks the same value.
 */
inline void BuildScalarTestData(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType amId, usize blockSize, const std::string& arrayName = "ScalarData", bool wrapBoundary = false)
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  const DataPath scalarPath = ds.getDataPathsForId(amId)[0].createChildPath(arrayName);
  auto scalarDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, scalarPath, cellShape, {1}, IDataAction::Mode::Execute);
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
          // The last block on each axis gets the first block value. Periodic
          // segmentation therefore merges the two boundary blocks.
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
 * @brief Creates quaternion, phase, and crystal-structure arrays for EBSD and C-axis tests.
 *
 * Each Z layer has one block-patterned orientation. All voxels use phase 1.
 * CrystalStructures uses 999 for unknown phase 0 and the selected value for
 * phase 1.
 *
 * @param ds Receives the test arrays.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param geomId Parent geometry identifier for the ensemble AttributeMatrix.
 * @param amId Parent CellData AttributeMatrix identifier.
 * @param crystalStructure Crystal structure for phase 1 (1 = Cubic_High, 0 = Hexagonal_High).
 * @param blockSize Number of voxels on each block edge.
 * @param wrapBoundary True to give opposite boundary blocks the same orientation.
 */
inline void BuildOrientationTestData(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType geomId, DataObject::IdType amId, uint32 crystalStructure, usize blockSize,
                                     bool wrapBoundary = false)
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  const DataPath amPath = ds.getDataPathsForId(amId)[0];
  const DataPath quatsPath = amPath.createChildPath("Quats");
  auto quatsDataStore = DataStoreUtilities::CreateDataStore<float32>(ds, quatsPath, cellShape, {4}, IDataAction::Mode::Execute);
  auto* quatsArray = DataArray<float32>::Create(ds, "Quats", quatsDataStore, amId);
  auto& quatsStore = quatsArray->getDataStoreRef();

  const DataPath phasesPath = amPath.createChildPath("Phases");
  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(ds, phasesPath, cellShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(ds, "Phases", phasesDataStore, amId);
  auto& phasesStore = phasesArray->getDataStoreRef();

  constexpr float32 k_DegToRad = 3.14159265358979323846f / 180.0f;

  const usize blocksPerX = (dimX + blockSize - 1) / blockSize;
  const usize blocksPerY = (dimY + blockSize - 1) / blockSize;
  const usize blocksPerZ = (dimZ + blockSize - 1) / blockSize;
  const usize numBlocks = blocksPerX * blocksPerY * blocksPerZ;

  // The Hamilton product treats each input quaternion as (w, x, y, z).
  auto quatMul = [](const std::array<float32, 4>& a, const std::array<float32, 4>& b) -> std::array<float32, 4> {
    return {a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3], a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2], a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
            a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]};
  };

  std::vector<std::array<float32, 4>> blockQuats(numBlocks);

  // Each Z layer uses one X-axis rotation, so all blocks in that layer merge.
  // The layers cycle through 0, 30, and 60 degrees. Their vector-scalar
  // quaternions are [0,0,0,1], [0.259,0,0,0.966], and [0.5,0,0,0.866].
  // The related C axes are [0,0,1], [0,0.5,0.866], and [0,0.866,0.5].
  // Adjacent layers remain separate because 30 degrees exceeds the 5-degree tolerance.
  //
  // For nonperiodic tests, block (1,1,1) uses 0 degrees instead of 30 degrees.
  // The block merges with its Z=0 face neighbor but not with other Z=1 blocks.
  // This pattern produces three features for three or eight blocks per axis.
  // Periodic layers with matching angles merge across the volume boundary.
  constexpr std::array<float32, 3> k_LayerAngles = {0.0f, 30.0f, 60.0f};

  for(usize bz = 0; bz < blocksPerZ; bz++)
  {
    const usize layerIdx = bz % 3;
    const float32 halfAngle = k_LayerAngles[layerIdx] * k_DegToRad * 0.5f;
    // EBSDLib stores quaternions in vector-scalar order (x, y, z, w).
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

  // The selected center block uses the Z=0 angle. It therefore merges through
  // face neighbor (1,1,0) into the Z=0 layer.
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

  // The ensemble arrays provide the crystal structure for phase 1.
  const ShapeType ensembleTupleShape = {2};
  auto* ensembleAM = AttributeMatrix::Create(ds, "CellEnsembleData", ensembleTupleShape, geomId);
  const DataPath crystalStructsPath = ds.getDataPathsForId(geomId)[0].createChildPath("CellEnsembleData").createChildPath("CrystalStructures");
  auto crystalDataStore = DataStoreUtilities::CreateDataStore<uint32>(ds, crystalStructsPath, ensembleTupleShape, {1}, IDataAction::Mode::Execute);
  auto* crystalStructsArray = DataArray<uint32>::Create(ds, "CrystalStructures", crystalDataStore, ensembleAM->getId());
  auto& crystalStructsStore = crystalStructsArray->getDataStoreRef();
  crystalStructsStore[0] = 999; // Phase 0: Unknown
  crystalStructsStore[1] = crystalStructure;
}

/**
 * @brief Creates a mask with value 1 inside a centered sphere and 0 outside.
 *
 * The radius is 80 percent of half the smallest dimension. A 200 by 200 by
 * 200 volume therefore has an 80-voxel radius.
 *
 * @param ds Receives the mask array.
 * @param cellShape Tuple shape {Z, Y, X}.
 * @param amId Parent AttributeMatrix identifier.
 * @param maskName Mask array name.
 */
inline void BuildSphericalMask(DataStructure& ds, const ShapeType& cellShape, DataObject::IdType amId, const std::string& maskName = "Mask")
{
  const usize dimZ = cellShape[0];
  const usize dimY = cellShape[1];
  const usize dimX = cellShape[2];

  const DataPath maskPath = ds.getDataPathsForId(amId)[0].createChildPath(maskName);
  auto maskDataStore = DataStoreUtilities::CreateDataStore<uint8>(ds, maskPath, cellShape, {1}, IDataAction::Mode::Execute);
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
 * @brief Verifies masked segmentation output.
 *
 * The check requires feature identifier 0 for masked voxels and a positive
 * identifier for unmasked voxels. It also requires both regions and at least
 * one output feature.
 *
 * @param ds Contains the segmentation results.
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
 * @brief Verifies block-patterned segmentation output.
 *
 * The check requires one feature for each block. All voxels in one block must
 * share an identifier, and different blocks must have different identifiers.
 *
 * @param ds Contains the segmentation results.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param blockSize Number of voxels on each block edge.
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

  // The Active array includes reserved feature 0.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(activePath));
  const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
  REQUIRE(actives.getNumberOfTuples() == expectedFeatures + 1);

  // Each block must use one positive feature identifier.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(featureIdsPath));
  const auto& featureIds = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();

  // The map records the first feature identifier found in each block.
  std::unordered_map<usize, int32> blockToFeature;
  // The set verifies that different blocks have different identifiers.
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

        REQUIRE(featureId > 0); // Each voxel must belong to a feature.

        auto it = blockToFeature.find(blockIdx);
        if(it == blockToFeature.end())
        {
          blockToFeature[blockIdx] = featureId;
          usedFeatureIds.insert(featureId);
        }
        else
        {
          REQUIRE(it->second == featureId); // All voxels in a block must use the same identifier.
        }
      }
    }
  }

  // Each block must have a unique feature identifier.
  REQUIRE(usedFeatureIds.size() == expectedFeatures);
}

/**
 * @brief Verifies periodic segmentation when opposite boundary blocks have matching data.
 *
 * Periodic wrapping merges the first and last block on each axis. The expected
 * feature count is `(blocksPerX - 1) * (blocksPerY - 1) * (blocksPerZ - 1)`.
 *
 * @param ds Contains the segmentation results.
 * @param dims Voxel dimensions {X, Y, Z}.
 * @param blockSize Number of voxels on each block edge.
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

  // The Active array includes reserved feature 0.
  REQUIRE_NOTHROW(ds.getDataRefAs<UInt8Array>(activePath));
  const auto& actives = ds.getDataRefAs<UInt8Array>(activePath);
  REQUIRE(actives.getNumberOfTuples() == expectedFeatures + 1);

  // Each periodic block group must use one positive feature identifier.
  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(featureIdsPath));
  const auto& featureIds = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& featureStore = featureIds.getDataStoreRef();

  // The map records the first feature identifier found in each periodic group.
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

        // The last block on each axis maps to the first periodic block.
        const usize pbx = bx % periodicBlocksX;
        const usize pby = by % periodicBlocksY;
        const usize pbz = bz % periodicBlocksZ;
        const usize periodicBlockIdx = pbz * periodicBlocksY * periodicBlocksX + pby * periodicBlocksX + pbx;

        const int32 featureId = featureStore.getValue(voxelIdx);
        REQUIRE(featureId > 0); // Each voxel must belong to a feature.

        auto it = blockToFeature.find(periodicBlockIdx);
        if(it == blockToFeature.end())
        {
          blockToFeature[periodicBlockIdx] = featureId;
          usedFeatureIds.insert(featureId);
        }
        else
        {
          REQUIRE(it->second == featureId); // Matching periodic blocks must use one identifier.
        }
      }
    }
  }

  // Each periodic block group must have a unique feature identifier.
  REQUIRE(usedFeatureIds.size() == expectedFeatures);
}

/**
 * @brief Verifies that a SegmentFeatures filter returns error -87000 for an empty mask.
 *
 * The test creates a 3 by 3 by 3 grid and masks all voxels.
 *
 * @tparam FilterT Specifies the SegmentFeatures filter type.
 * @tparam SetupArgsFn Specifies the argument-setup callable type.
 * @param setupArgs Adds filter-specific arguments for the test paths.
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
 * @brief Verifies FaceEdgeVertex connectivity for vertex-connected and edge-connected regions.
 *
 * The 3 by 3 by 3 geometry contains one vertex-connected pair and one
 * edge-connected pair. Face connectivity produces five features, including
 * the background. FaceEdgeVertex connectivity produces three features by
 * merging both pairs.
 *
 * @tparam FilterT Specifies the SegmentFeatures filter type.
 * @tparam SetupFaceFn Specifies the face-connectivity setup callable type.
 * @tparam SetupFevFn Specifies the FaceEdgeVertex setup callable type.
 * @param setupFaceArgs Adds filter arguments with face connectivity.
 * @param setupFevArgs Adds filter arguments with FaceEdgeVertex connectivity.
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

  // Face connectivity leaves regions A, B, C, and D separate from the background.
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

  // FaceEdgeVertex connectivity merges A with B and C with D.
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

  // Each connected pair must share one identifier, and the two pairs must differ.
  const auto& fids = ds.getDataRefAs<Int32Array>(featureIdsPath);
  const auto& fidsStore = fids.getDataStoreRef();
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 0) == fidsStore.getValue(1 * 9 + 1 * 3 + 1)); // A == B (vertex merge)
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 2) == fidsStore.getValue(1 * 9 + 1 * 3 + 2)); // C == D (edge merge)
  REQUIRE(fidsStore.getValue(0 * 9 + 0 * 3 + 0) != fidsStore.getValue(0 * 9 + 0 * 3 + 2)); // A != C (different values)
}

/**
 * @brief Executes a SegmentFeatures filter and verifies its exemplar output.
 *
 * The test can check the feature count. It also compares feature identifiers
 * and verifies tuple-dimension inheritance for all applicable arrays.
 *
 * @tparam FilterT Specifies the SegmentFeatures filter type.
 * @tparam SetupArgsFn Specifies the argument-setup callable type.
 * @param dataStructure Contains the exemplar input and receives computed output.
 * @param computedFeatureIdsPath Computed FeatureIds array path.
 * @param activesPath Computed Active array path.
 * @param exemplarFeatureIdsPath Exemplar FeatureIds array path.
 * @param expectedFeatureCount Expected Active tuple count, or 0 to omit this check.
 * @param setupArgs Adds the filter-specific arguments.
 * @param tupleCheckIgnoredPaths Paths omitted from the tuple-dimension check.
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

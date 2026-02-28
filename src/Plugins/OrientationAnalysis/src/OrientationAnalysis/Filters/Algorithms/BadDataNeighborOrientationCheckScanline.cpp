#include "BadDataNeighborOrientationCheckScanline.hpp"

#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckScanline::BadDataNeighborOrientationCheckScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                 const BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckScanline::~BadDataNeighborOrientationCheckScanline() noexcept = default;

// -----------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheckScanline::operator()()
{
  const float misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize totalPoints = quats.getNumberOfTuples();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    return MakeErrorResult(-54900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
  }

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const int64 xyStride = dims[0] * dims[1];

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<int32> neighborCount(totalPoints, 0);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  // ===== Phase 1: Count matching good neighbors for each bad voxel =====
  // Chunk-sequential iteration for OOC efficiency (no-op for in-core)
  auto& quatsStore = quats.getDataStoreRef();
  const uint64 numChunks = quatsStore.getNumberOfChunks();

  for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
  {
    quatsStore.loadChunk(chunkIdx);

    const auto chunkLowerBounds = quatsStore.getChunkLowerBounds(chunkIdx);
    const auto chunkUpperBounds = quatsStore.getChunkUpperBounds(chunkIdx);

    for(usize zIdx = chunkLowerBounds[0]; zIdx <= chunkUpperBounds[0]; zIdx++)
    {
      const int64 kStride = static_cast<int64>(zIdx) * xyStride;
      for(usize yIdx = chunkLowerBounds[1]; yIdx <= chunkUpperBounds[1]; yIdx++)
      {
        const int64 jStride = static_cast<int64>(yIdx) * dims[0];
        for(usize xIdx = chunkLowerBounds[2]; xIdx <= chunkUpperBounds[2]; xIdx++)
        {
          const int64 voxelIndex = kStride + jStride + static_cast<int64>(xIdx);

          throttledMessenger.sendThrottledMessage([&] { return fmt::format("Processing Data {:.2f}% completed", CalculatePercentComplete(voxelIndex, totalPoints)); });

          if(!maskCompare->isTrue(voxelIndex))
          {
            ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
            quat1.positiveOrientation();
            const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(static_cast<int64>(xIdx), static_cast<int64>(yIdx), static_cast<int64>(zIdx), dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

              if(maskCompare->isTrue(neighborPoint))
              {
                if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
                {
                  ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
                  quat2.positiveOrientation();
                  ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                  if(axisAngle[3] < misorientationTolerance)
                  {
                    neighborCount[voxelIndex]++;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // ===== Phase 2: Iteratively flip bad voxels using chunk-sequential multi-pass scans =====
  // For each level, repeatedly scan the volume sequentially. On each pass, flip eligible
  // voxels and update their neighbors' counts. Repeat until no flips occur.
  // This avoids the random access pattern of worklist-based processing.
  //
  // Chunk-skip optimization: Before loading a chunk from disk, scan the in-memory
  // neighborCount vector for that chunk's voxel range. If no voxel has
  // neighborCount >= currentLevel, skip the chunk entirely (no disk I/O).
  // Flipped voxels get neighborCount = -1 to prevent false positives.
  constexpr int32 startLevel = 6;
  const int32 totalLevels = startLevel - m_InputValues->NumberOfNeighbors + 1;

  for(int32 currentLevel = startLevel; currentLevel >= m_InputValues->NumberOfNeighbors; currentLevel--)
  {
    bool changed = true;
    int32 passCount = 0;

    while(changed)
    {
      changed = false;
      passCount++;

      throttledMessenger.sendThrottledMessage([&] { return fmt::format("Level '{}' of '{}' || Pass {}", (startLevel - currentLevel) + 1, totalLevels, passCount); });

      for(uint64 chunkIdx = 0; chunkIdx < numChunks; chunkIdx++)
      {
        const auto chunkLowerBounds = quatsStore.getChunkLowerBounds(chunkIdx);
        const auto chunkUpperBounds = quatsStore.getChunkUpperBounds(chunkIdx);

        // Compute the flat voxel index range for this chunk and check if any
        // voxel in the chunk could be eligible (neighborCount >= currentLevel).
        // This check uses only the in-memory neighborCount vector — no disk I/O.
        const usize chunkStartIdx = chunkLowerBounds[0] * static_cast<usize>(xyStride) + chunkLowerBounds[1] * static_cast<usize>(dims[0]) + chunkLowerBounds[2];
        const usize chunkEndIdx = chunkUpperBounds[0] * static_cast<usize>(xyStride) + chunkUpperBounds[1] * static_cast<usize>(dims[0]) + chunkUpperBounds[2];

        bool hasEligible = false;
        for(usize i = chunkStartIdx; i <= chunkEndIdx; i++)
        {
          if(neighborCount[i] >= currentLevel)
          {
            hasEligible = true;
            break;
          }
        }

        if(!hasEligible)
        {
          continue;
        }

        quatsStore.loadChunk(chunkIdx);

        for(usize zIdx = chunkLowerBounds[0]; zIdx <= chunkUpperBounds[0]; zIdx++)
        {
          const int64 kStride = static_cast<int64>(zIdx) * xyStride;
          for(usize yIdx = chunkLowerBounds[1]; yIdx <= chunkUpperBounds[1]; yIdx++)
          {
            const int64 jStride = static_cast<int64>(yIdx) * dims[0];
            for(usize xIdx = chunkLowerBounds[2]; xIdx <= chunkUpperBounds[2]; xIdx++)
            {
              const int64 voxelIndex = kStride + jStride + static_cast<int64>(xIdx);

              if(!maskCompare->isTrue(voxelIndex) && neighborCount[voxelIndex] >= currentLevel)
              {
                maskCompare->setValue(voxelIndex, true);
                neighborCount[voxelIndex] = -1; // Mark as flipped to prevent false positives in chunk-skip check
                changed = true;

                ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
                quat1.positiveOrientation();
                const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

                std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(static_cast<int64>(xIdx), static_cast<int64>(yIdx), static_cast<int64>(zIdx), dims);
                for(const auto& faceIndex : faceNeighborInternalIdx)
                {
                  if(!isValidFaceNeighbor[faceIndex])
                  {
                    continue;
                  }

                  const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

                  if(!maskCompare->isTrue(neighborPoint))
                  {
                    if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
                    {
                      ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
                      quat2.positiveOrientation();
                      ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                      if(axisAngle[3] < misorientationTolerance)
                      {
                        neighborCount[neighborPoint]++;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return {};
}

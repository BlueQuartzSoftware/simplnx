/**
 * @file ErodeDilateBadData.cpp
 * @brief Iterative morphological erosion/dilation of bad (FeatureId == 0) voxels,
 *        optimized for out-of-core (OOC) data stores via Z-slice buffered I/O.
 *
 * ## High-Level Flow (per iteration)
 *
 * 1. **Initialize rolling window** -- Load FeatureId Z-slices 0 and 1 into
 *    slots 1 (current) and 2 (next) of the three-element window.
 *
 * 2. **Scan every voxel** (Z-major, then Y, then X):
 *    - For each bad voxel (featureId == 0), examine its 6 face neighbors using
 *      the in-memory rolling-window buffers.
 *    - **Dilate**: If a neighbor is good (featureId > 0), mark that good
 *      neighbor to be overwritten with the bad voxel's data.
 *    - **Erode**: Tally the good neighbors and record the most common one;
 *      mark the bad voxel to be overwritten with that neighbor's data.
 *    - Marks are stored in three O(sliceSize) arrays (one per rolling-window
 *      slot) rather than a single O(totalPoints) array.
 *
 * 3. **Deferred transfer** -- After processing each Z-slice, the marks for
 *    slice z-1 are complete (because only voxels at z-2, z-1, and z can
 *    affect z-1). Commit those marks via SliceBufferedTransferOneZ, which
 *    reads the source and destination slices in bulk, applies the copies
 *    in-memory, and writes back.
 *
 * 4. **Rotate windows** -- Swap mark arrays and FeatureId slices forward by
 *    one position. Clear the newly vacated slot for the next Z-layer.
 *
 * 5. **Flush final slice** -- After the Z-loop exits, the last slice's marks
 *    are still pending; commit them.
 *
 * Each iteration must re-read FeatureIds from the store because the preceding
 * iteration's SliceBufferedTransferOneZ calls may have changed values.
 */

#include "ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ErodeDilateBadData::ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateBadData::~ErodeDilateBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateBadData::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64, 3> dims = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  // Precompute face-neighbor offsets: given a flat voxel index, adding one of
  // these offsets yields the flat index of the corresponding face neighbor.
  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // ---- Determine max FeatureId using sequential Z-slice reads ----
  // This avoids a full-volume random-access scan that would thrash OOC chunks.
  usize numFeatures = 0;
  {
    std::vector<int32> sliceBuf(sliceSize);
    for(int64 z = 0; z < dims[2]; z++)
    {
      featureIds.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(sliceBuf.data(), sliceSize));
      for(usize i = 0; i < sliceSize; i++)
      {
        if(sliceBuf[i] > static_cast<int32>(numFeatures))
        {
          numFeatures = sliceBuf[i];
        }
      }
    }
  }

  // featureCount is used during erosion to tally how many of each neighbor
  // FeatureId surround a bad voxel. It is sized to numFeatures+1 so that
  // featureCount[featureId] is directly addressable.
  std::vector<int32> featureCount(numFeatures + 1, 0);

  // ---- FeatureIds rolling window (3 Z-slices) ----
  // Slot 0 = z-1, slot 1 = z (current), slot 2 = z+1.
  // All face-neighbor FeatureId reads come from these buffers rather than
  // the underlying (potentially OOC) data store.
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) { featureIds.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(featureIdSlices[slot].data(), sliceSize)); };

  // Maps face-neighbor index (0=-Z, 1=-Y, 2=-X, 3=+X, 4=+Y, 5=+Z) to the
  // rolling-window slot that holds the neighbor's Z-slice.
  // -Z is in slot 0 (prev), -Y/-X/+X/+Y are in slot 1 (current), +Z is in slot 2 (next).
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // ---- Per-slice mark arrays (3 x sliceSize) ----
  // Each entry is -1 (no transfer needed) or the global flat index of the
  // source voxel whose data should be copied to this position.
  // marks[0] corresponds to Z-1, marks[1] to Z, marks[2] to Z+1.
  // This replaces a full-volume O(totalPoints) neighbor array with
  // O(3 * sliceSize) memory, which is critical for large datasets.
  std::array<std::vector<int64>, 3> marks;
  for(auto& m : marks)
  {
    m.resize(sliceSize);
  }

  // Collect all sibling arrays in the same Attribute Matrix as FeatureIds,
  // excluding user-specified ignored arrays. These will all be updated
  // during the transfer phase to stay consistent with FeatureId changes.
  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
  const usize dimZ = static_cast<usize>(dims[2]);

  // Commits one Z-slice of marks across all sibling arrays.
  // SliceBufferedTransferOneZ handles the bulk read/copy/write internally.
  auto transferSlice = [&](usize z, const std::vector<int64>& sliceMarks) {
    for(const auto& voxelArray : voxelArrays)
    {
      SliceBufferedTransferOneZ(*voxelArray, sliceMarks, sliceSize, z, dimZ);
    }
  };

  // ---- Main iteration loop ----
  // Each iteration performs one complete pass of the morphological operation.
  // The rolling window and mark arrays are reset at the start of each
  // iteration because the previous pass's transfers alter the data.
  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    // Clear all mark arrays for this iteration
    for(auto& m : marks)
    {
      std::fill(m.begin(), m.end(), -1);
    }

    // Initialize FeatureId rolling window: z=0 -> slot 1, z=1 -> slot 2
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    // ---- Z-slice scan loop ----
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance the rolling window forward by one Z-slice.
      // After this swap: slot 0 = old current (z-1), slot 1 = old next (z),
      // slot 2 = newly loaded z+1.
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readFeatureIdSlice(zIdx + 1, 2);
        }
      }

      // ---- Inner XY scan: identify marks for bad voxels in this Z-slice ----
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];

          // Only process bad voxels (featureId == 0)
          if(featureName == 0)
          {
            int32 most = 0;

            // Determine which face neighbors are within the volume bounds,
            // then mask off directions the user has disabled.
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            if(!m_InputValues->XDirOn)
            {
              isValidFaceNeighbor[k_NegativeXNeighbor] = false;
              isValidFaceNeighbor[k_PositiveXNeighbor] = false;
            }
            if(!m_InputValues->YDirOn)
            {
              isValidFaceNeighbor[k_NegativeYNeighbor] = false;
              isValidFaceNeighbor[k_PositiveYNeighbor] = false;
            }
            if(!m_InputValues->ZDirOn)
            {
              isValidFaceNeighbor[k_NegativeZNeighbor] = false;
              isValidFaceNeighbor[k_PositiveZNeighbor] = false;
            }

            // Global flat index of this voxel (used as source index in marks)
            const int64 voxelIndex = xIdx + yIdx * dims[0] + zIdx * static_cast<int64>(sliceSize);

            // Map each face neighbor to its position within the appropriate
            // rolling-window slice buffer. For -Z and +Z the XY position is
            // the same (inSlice); for in-plane neighbors it shifts by +/-1
            // in the X or Y direction.
            const std::array<usize, 6> neighborInSlice = {
                inSlice,                                         // -Z
                static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                inSlice                                          // +Z
            };

            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              // Compute the global flat index of this neighbor and read its
              // FeatureId from the rolling-window buffer (not the OOC store).
              const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];

              if(m_InputValues->Operation == detail::k_DilateIndex && feature > 0)
              {
                // DILATION: Mark the good NEIGHBOR to receive this bad voxel's
                // data. The neighbor lives in rolling-window slot
                // k_NeighborSlot[faceIndex], so we mark that slot's array.
                marks[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]] = voxelIndex;
              }
              if(feature > 0 && m_InputValues->Operation == detail::k_ErodeIndex)
              {
                // EROSION: Tally this good neighbor's FeatureId and track
                // which one is the most common ("majority vote").
                featureCount[feature]++;
                const int32 current = featureCount[feature];
                if(current > most)
                {
                  most = current;
                  // Mark this bad voxel to receive the best neighbor's data
                  marks[1][inSlice] = neighborPoint;
                }
              }
            }

            // Reset featureCount entries for this voxel's neighbors so the
            // array is clean for the next bad voxel (avoids a full memset).
            if(m_InputValues->Operation == detail::k_ErodeIndex)
            {
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }
                const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
                featureCount[feature] = 0;
              }
            }
          }
        }
      }

      // ---- Deferred transfer for slice z-1 ----
      // At this point all voxels that could write marks into slice z-1 have
      // been processed (only voxels in slices z-2, z-1, and z can affect z-1
      // via face-neighbor relationships). It is now safe to commit z-1.
      if(zIdx > 0)
      {
        transferSlice(static_cast<usize>(zIdx - 1), marks[0]);
      }

      // ---- Rotate mark arrays forward ----
      // marks[0] <- old marks[1] (becomes the "previous" for next iteration)
      // marks[1] <- old marks[2] (becomes the "current")
      // marks[2] <- cleared (ready for the next Z+1 layer)
      std::swap(marks[0], marks[1]);
      std::swap(marks[1], marks[2]);
      std::fill(marks[2].begin(), marks[2].end(), -1);
    }

    // ---- Flush final Z-slice ----
    // After the Z-loop exits, the last slice's marks are in marks[0] (due to
    // the rotation) and have not yet been committed.
    if(dims[2] > 0)
    {
      transferSlice(static_cast<usize>(dims[2] - 1), marks[0]);
    }
  }

  return {};
}

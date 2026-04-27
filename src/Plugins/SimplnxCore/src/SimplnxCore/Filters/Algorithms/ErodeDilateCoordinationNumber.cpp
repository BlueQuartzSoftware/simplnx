/**
 * @file ErodeDilateCoordinationNumber.cpp
 * @brief Coordination-number-based boundary smoothing of good/bad voxels,
 *        optimized for out-of-core (OOC) data stores via Z-slice buffered I/O.
 *
 * ## High-Level Flow (per pass)
 *
 * 1. **Initialize rolling window** -- Load FeatureId Z-slices 0 and 1 into
 *    the three-element window (slots 1 and 2).
 *
 * 2. **Scan every voxel** (Z-major, then Y, then X):
 *    - For each voxel on a good/bad boundary, count face neighbors of the
 *      opposite type (the "coordination number") and record the most common
 *      neighbor FeatureId.
 *    - Store the coordination number and best-neighbor index in per-slice
 *      arrays rather than full-volume arrays.
 *
 * 3. **Deferred transfer** -- After processing Z-slice z, commit the marks
 *    for z-1 (which are now complete). Only voxels whose coordination number
 *    meets or exceeds the user's threshold are actually transferred.
 *
 * 4. **Rotate windows** -- Shift rolling-window buffers and per-slice arrays
 *    forward by one Z-layer.
 *
 * 5. **Flush final slice** -- Commit the last slice's marks after the Z-loop.
 *
 * 6. **Repeat** until no voxels were modified (if Loop is true) or after
 *    one pass (if Loop is false).
 */

#include "ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ErodeDilateCoordinationNumberInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::~ErodeDilateCoordinationNumber() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateCoordinationNumber::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64, 3> dims = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  // Precompute face-neighbor index offsets and iteration order
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  // Collect all sibling arrays that should be updated during the transfer phase
  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
  const usize dimZ = static_cast<usize>(dims[2]);

  // ---- Determine max FeatureId using sequential Z-slice reads ----
  // Sequential bulk reads avoid OOC chunk thrashing.
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  usize numFeatures = 0;
  {
    std::vector<int32> sliceBuf(sliceSize);
    for(int64 z = 0; z < dims[2]; z++)
    {
      featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(sliceBuf.data(), sliceSize));
      for(usize i = 0; i < sliceSize; i++)
      {
        if(sliceBuf[i] > static_cast<int32>(numFeatures))
        {
          numFeatures = sliceBuf[i];
        }
      }
    }
  }

  // Per-voxel neighbor feature tally, sized so featureCount[featureId] is
  // directly addressable. Reset after each voxel to avoid a full memset.
  std::vector<int32> featureCount(numFeatures + 1, 0);
  bool keepGoing = true;
  int32 counter = 1;

  // ---- FeatureIds rolling window (3 Z-slices) ----
  // Slot 0 = z-1 (previous), slot 1 = z (current), slot 2 = z+1 (next).
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) { featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(featureIdSlices[slot].data(), sliceSize)); };

  // Maps face-neighbor index to rolling-window slot:
  // -Z -> slot 0, -Y/-X/+X/+Y -> slot 1 (same Z), +Z -> slot 2
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // ---- Per-slice neighbor marks: 3 x O(sliceSize) ----
  // Each entry is -1 (no transfer) or the global flat index of the source voxel.
  // Replaces the O(totalPoints) full-volume neighbors array.
  std::array<std::vector<int64>, 3> sliceNeighbors;
  for(auto& sn : sliceNeighbors)
  {
    sn.resize(sliceSize, -1);
  }

  // ---- Per-slice coordination numbers: 3 x O(sliceSize) ----
  // Tracks the coordination number for each voxel in the rolling window.
  // Only voxels whose coordination number meets the threshold will be
  // transferred during the commit phase.
  std::array<std::vector<int32>, 3> sliceCoordination;
  for(auto& sc : sliceCoordination)
  {
    sc.resize(sliceSize, 0);
  }

  // Commits one Z-slice worth of marks, but only for voxels whose coordination
  // number meets or exceeds the user's threshold. This filtering step is what
  // distinguishes this algorithm from simple erosion/dilation: low-coordination
  // boundary voxels are left alone.
  auto transferSlice = [&](usize z, const std::vector<int64>& marks, const std::vector<int32>& coord) {
    std::vector<int64> filteredMarks(sliceSize, -1);
    for(usize i = 0; i < sliceSize; i++)
    {
      if(coord[i] >= m_InputValues->CoordinationNumber && coord[i] > 0)
      {
        filteredMarks[i] = marks[i];
        counter++;
      }
    }
    for(const auto& voxelArray : voxelArrays)
    {
      SliceBufferedTransferOneZ(*voxelArray, filteredMarks, sliceSize, z, dimZ);
    }
  };

  // ---- Main pass loop ----
  // Repeats until either (a) no voxels were modified this pass, or
  // (b) Loop is false and a single pass has completed.
  while(counter > 0 && keepGoing)
  {
    counter = 0;
    if(!m_InputValues->Loop)
    {
      keepGoing = false;
    }

    // Clear per-slice tracking arrays for this pass
    for(auto& sn : sliceNeighbors)
    {
      std::fill(sn.begin(), sn.end(), -1);
    }
    for(auto& sc : sliceCoordination)
    {
      std::fill(sc.begin(), sc.end(), 0);
    }

    // Re-initialize rolling window from the (potentially modified) store
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    // ---- Z-slice scan loop ----
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance the FeatureId rolling window
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readFeatureIdSlice(zIdx + 1, 2);
        }
      }

      // ---- Inner XY scan ----
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = dims[0] * dims[1] * zIdx + dims[0] * yIdx + xIdx;
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          int32 coordination = 0;
          int32 most = 0;

          const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

          // Map each face neighbor to its position within its rolling-window slice
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

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];

            // A voxel is on the boundary if it and its neighbor have opposite
            // good/bad status (one is 0, the other is > 0).
            if((featureName > 0 && feature == 0) || (featureName == 0 && feature > 0))
            {
              coordination = coordination + 1;
              featureCount[feature]++;
              const int32 current = featureCount[feature];
              if(current > most)
              {
                most = current;
                // Record this neighbor as the best replacement source
                sliceNeighbors[1][inSlice] = neighborPoint;
              }
            }
          }
          // Store the computed coordination number for the transfer-filter step
          sliceCoordination[1][inSlice] = coordination;

          // Reset featureCount entries touched by this voxel's neighbors
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
            if(feature > 0)
            {
              featureCount[feature] = 0;
            }
          }
        }
      }

      // ---- Deferred transfer for slice z-1 ----
      // After processing slice z, all marks for z-1 are complete.
      if(zIdx > 0)
      {
        transferSlice(static_cast<usize>(zIdx - 1), sliceNeighbors[0], sliceCoordination[0]);
      }

      // ---- Rotate per-slice arrays forward ----
      std::swap(sliceNeighbors[0], sliceNeighbors[1]);
      std::swap(sliceNeighbors[1], sliceNeighbors[2]);
      std::fill(sliceNeighbors[2].begin(), sliceNeighbors[2].end(), -1);

      std::swap(sliceCoordination[0], sliceCoordination[1]);
      std::swap(sliceCoordination[1], sliceCoordination[2]);
      std::fill(sliceCoordination[2].begin(), sliceCoordination[2].end(), 0);
    }

    // ---- Flush final Z-slice ----
    if(dims[2] > 0)
    {
      transferSlice(static_cast<usize>(dims[2] - 1), sliceNeighbors[0], sliceCoordination[0]);
    }
  }

  return {};
}

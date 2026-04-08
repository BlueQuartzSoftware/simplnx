#include "NeighborOrientationCorrelation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::NeighborOrientationCorrelation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               NeighborOrientationCorrelationInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::~NeighborOrientationCorrelation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> NeighborOrientationCorrelation::operator()()
{
  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ConfidenceIndexArrayPath);
  auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Cache ensemble-level arrays locally to avoid per-element virtual dispatch in hot loops
  const auto& crystalStructuresStore = crystalStructuresArray.getDataStoreRef();
  const usize numPhases = crystalStructuresStore.getNumberOfTuples();
  std::vector<uint32> crystalStructures(numPhases);
  crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), numPhases));

  const auto& ciStore = confidenceIndex.getDataStoreRef();
  const auto& phaseStore = cellPhases.getDataStoreRef();
  const auto& quatStore = quats.getDataStoreRef();

  usize totalPoints = confidenceIndex.getNumberOfTuples();

  float32 misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float32> / 180.0f;

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);

  std::array<int32, 6> neighborSimCount = {};
  const int32 startLevel = 6;

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  // Z-slice buffering: read 3 adjacent Z-slices of the most-accessed arrays
  // into local memory to eliminate OOC chunk thrashing. The algorithm accesses
  // each voxel's 6 face neighbors, requiring data from z-1, z, and z+1 slices.
  // By buffering these slices, all neighbor lookups become local memory reads.
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling window: slot 0 = z-1, slot 1 = z (current), slot 2 = z+1
  std::array<std::vector<float32>, 3> quatSlices;
  std::array<std::vector<int32>, 3> phaseSlices;
  std::vector<float32> ciSlice(sliceSize);

  for(auto& qs : quatSlices)
  {
    qs.resize(sliceSize * 4);
  }
  for(auto& ps : phaseSlices)
  {
    ps.resize(sliceSize);
  }

  // Bulk-read a Z-slice using copyIntoBuffer for OOC efficiency
  auto readQuatSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize * 4;
    quatStore.copyIntoBuffer(zOffset, nonstd::span<float32>(quatSlices[slot].data(), sliceSize * 4));
  };

  auto readPhaseSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    phaseStore.copyIntoBuffer(zOffset, nonstd::span<int32>(phaseSlices[slot].data(), sliceSize));
  };

  auto readCISlice = [&](int64 z) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    ciStore.copyIntoBuffer(zOffset, nonstd::span<float32>(ciSlice.data(), sliceSize));
  };

  // Per-slice best neighbor marks (replaces O(totalPoints) bestNeighbor array)
  std::vector<int64> sliceBestNeighbor(sliceSize, -1);
  const usize dimZ = static_cast<usize>(dims[2]);
  std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->ConfidenceIndexArrayPath, m_InputValues->IgnoredDataArrayPaths);

  for(int32 currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    usize processedVoxels = 0;

    // Initialize rolling window: load z=0 into slot 1, z=1 into slot 2
    readQuatSlice(0, 1);
    readPhaseSlice(0, 1);
    if(dims[2] > 1)
    {
      readQuatSlice(1, 2);
      readPhaseSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2] && !m_ShouldCancel; zIdx++)
    {
      // Advance rolling window for z > 0
      if(zIdx > 0)
      {
        std::swap(quatSlices[0], quatSlices[1]);
        std::swap(quatSlices[1], quatSlices[2]);
        std::swap(phaseSlices[0], phaseSlices[1]);
        std::swap(phaseSlices[1], phaseSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readQuatSlice(zIdx + 1, 2);
          readPhaseSlice(zIdx + 1, 2);
        }
      }

      readCISlice(zIdx);

      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          int64 voxelIndex = xIdx + yIdx * dims[0] + zIdx * static_cast<int64>(sliceSize);
          usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);

          if(processedVoxels % 10000 == 0)
          {
            throttledMessenger.sendThrottledMessage([&]() {
              return fmt::format("Level '{}' of '{}' || Processing Data {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->Level,
                                 CalculatePercentComplete(processedVoxels, totalPoints));
            });
          }

          if(ciSlice[inSlice] < m_InputValues->MinConfidence)
          {
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

            // Pre-read all valid neighbor quats and phases into local arrays.
            // Neighbor buffer slots: 0=-Z, 1=-Y(same z), 2=-X(same z), 3=+X(same z), 4=+Y(same z), 5=+Z
            // slot mapping: -Z→0, same-z→1, +Z→2
            constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};
            const std::array<int64, 6> neighborBufX = {xIdx, xIdx, xIdx - 1, xIdx + 1, xIdx, xIdx};
            const std::array<int64, 6> neighborBufY = {yIdx, yIdx - 1, yIdx, yIdx, yIdx + 1, yIdx};

            std::array<ebsdlib::QuatD, 6> nQuats;
            std::array<int32, 6> nPhases = {};

            for(usize f = 0; f < k_FaceNeighborCount; f++)
            {
              if(isValidFaceNeighbor[f])
              {
                usize nIdx = static_cast<usize>(neighborBufY[f] * dims[0] + neighborBufX[f]);
                usize nIdx4 = nIdx * 4;
                nPhases[f] = phaseSlices[k_NeighborSlot[f]][nIdx];
                nQuats[f] =
                    ebsdlib::QuatD(quatSlices[k_NeighborSlot[f]][nIdx4], quatSlices[k_NeighborSlot[f]][nIdx4 + 1], quatSlices[k_NeighborSlot[f]][nIdx4 + 2], quatSlices[k_NeighborSlot[f]][nIdx4 + 3]);
              }
            }

            // Compute neighbor-neighbor similarity counts
            neighborSimCount.fill(0);

            for(usize faceIndexJ = 0; faceIndexJ < k_FaceNeighborCount; faceIndexJ++)
            {
              if(!isValidFaceNeighbor[faceIndexJ])
              {
                continue;
              }

              for(usize faceIndexK = faceIndexJ + 1; faceIndexK < k_FaceNeighborCount; faceIndexK++)
              {
                if(!isValidFaceNeighbor[faceIndexK])
                {
                  continue;
                }

                if(nPhases[faceIndexK] == nPhases[faceIndexJ] && nPhases[faceIndexK] > 0)
                {
                  uint32 laueClass = crystalStructures[nPhases[faceIndexK]];
                  ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(nQuats[faceIndexK], nQuats[faceIndexJ]);
                  if(axisAngle[3] < misorientationToleranceR)
                  {
                    neighborSimCount[faceIndexJ]++;
                    neighborSimCount[faceIndexK]++;
                  }
                }
              }
            }

            // Find the best neighbor (last valid face with positive similarity count)
            for(usize faceIndex = 0; faceIndex < k_FaceNeighborCount; faceIndex++)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              if(neighborSimCount[faceIndex] > 0)
              {
                sliceBestNeighbor[inSlice] = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              }
            }
          }

          processedVoxels++;
        }
      }

      // Transfer this Z-slice immediately (bestNeighbor only marks the current voxel)
      for(const auto& dataArrayPtr : voxelArrays)
      {
        SliceBufferedTransferOneZ(*dataArrayPtr, sliceBestNeighbor, sliceSize, static_cast<usize>(zIdx), dimZ);
      }
      std::fill(sliceBestNeighbor.begin(), sliceBestNeighbor.end(), -1);
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    currentLevel = currentLevel - 1;
  }

  return {};
}

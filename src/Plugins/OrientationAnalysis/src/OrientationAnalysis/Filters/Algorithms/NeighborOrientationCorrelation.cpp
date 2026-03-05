#include "NeighborOrientationCorrelation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

class NeighborOrientationCorrelationTransferDataImpl
{
public:
  NeighborOrientationCorrelationTransferDataImpl() = delete;
  NeighborOrientationCorrelationTransferDataImpl(const NeighborOrientationCorrelationTransferDataImpl&) = default;

  // SAFETY: bestNeighbor must outlive the ParallelTaskAlgorithm that executes this functor.
  NeighborOrientationCorrelationTransferDataImpl(MessageHelper& messageHelper, size_t totalPoints, const std::vector<int64>& bestNeighbor, std::shared_ptr<IDataArray> dataArrayPtr)
  : m_MessageHelper(messageHelper)
  , m_TotalPoints(totalPoints)
  , m_BestNeighbor(bestNeighbor)
  , m_DataArrayPtr(dataArrayPtr)
  {
  }
  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelationTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(const NeighborOrientationCorrelationTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(NeighborOrientationCorrelationTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~NeighborOrientationCorrelationTransferDataImpl() = default;

  void operator()() const
  {
    ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
    std::string arrayName = m_DataArrayPtr->getName();
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)); });
      int64 neighbor = m_BestNeighbor[i];
      if(neighbor != -1)
      {
        m_DataArrayPtr->copyTuple(neighbor, i);
      }
    }
  }

private:
  MessageHelper& m_MessageHelper;
  size_t m_TotalPoints = 0;
  const std::vector<int64>& m_BestNeighbor;
  std::shared_ptr<IDataArray> m_DataArrayPtr;
};

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
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  size_t totalPoints = confidenceIndex.getNumberOfTuples();

  float misorientationToleranceR = m_InputValues->MisorientationTolerance * numbers::pi_v<float> / 180.0f;

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);

  std::array<int32, 6> neighborSimCount = {};
  std::vector<int64> bestNeighbor(totalPoints, -1);
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

  // Read a Z-slice of quats into a buffer slot (sequential access for OOC)
  auto readQuatSlice = [&](int64 z, usize slot) {
    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        usize inSlice = static_cast<usize>(y * dims[0] + x);
        int64 vi = x + y * dims[0] + z * static_cast<int64>(sliceSize);
        quatSlices[slot][inSlice * 4] = quats[vi * 4];
        quatSlices[slot][inSlice * 4 + 1] = quats[vi * 4 + 1];
        quatSlices[slot][inSlice * 4 + 2] = quats[vi * 4 + 2];
        quatSlices[slot][inSlice * 4 + 3] = quats[vi * 4 + 3];
      }
    }
  };

  auto readPhaseSlice = [&](int64 z, usize slot) {
    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        usize inSlice = static_cast<usize>(y * dims[0] + x);
        int64 vi = x + y * dims[0] + z * static_cast<int64>(sliceSize);
        phaseSlices[slot][inSlice] = cellPhases[vi];
      }
    }
  };

  auto readCISlice = [&](int64 z) {
    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        usize inSlice = static_cast<usize>(y * dims[0] + x);
        int64 vi = x + y * dims[0] + z * static_cast<int64>(sliceSize);
        ciSlice[inSlice] = confidenceIndex[vi];
      }
    }
  };

  for(int32 currentLevel = startLevel; currentLevel > m_InputValues->Level; currentLevel--)
  {
    std::fill(bestNeighbor.begin(), bestNeighbor.end(), -1);
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

          throttledMessenger.sendThrottledMessage([&]() {
            return fmt::format("Level '{}' of '{}' || Processing Data {:.2f}% completed", (startLevel - currentLevel) + 1, startLevel - m_InputValues->Level,
                               CalculatePercentComplete(processedVoxels, totalPoints));
          });

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
                bestNeighbor[voxelIndex] = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              }
            }
          }

          processedVoxels++;
        }
      }
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    // Build up a list of the DataArrays that we are going to operate on.
    std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->ConfidenceIndexArrayPath, m_InputValues->IgnoredDataArrayPaths);
    // Parallel per-array transfer: each task processes one entire array sequentially,
    // which keeps OOC chunk access patterns coherent within each array. Multiple arrays
    // run concurrently via ParallelTaskAlgorithm.
    ParallelTaskAlgorithm parallelTask;
    for(const auto& dataArrayPtr : voxelArrays)
    {
      parallelTask.execute(NeighborOrientationCorrelationTransferDataImpl(messageHelper, totalPoints, bestNeighbor, dataArrayPtr));
    }

    currentLevel = currentLevel - 1;
  }

  return {};
}

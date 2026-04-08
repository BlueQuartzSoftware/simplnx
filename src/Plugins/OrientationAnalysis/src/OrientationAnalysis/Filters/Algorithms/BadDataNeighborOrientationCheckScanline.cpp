#include "BadDataNeighborOrientationCheckScanline.hpp"

#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{
/**
 * @brief Computes misorientation between quat1 and the neighbor at neighborSliceIdx.
 * Returns true if same phase, phase > 0, and misorientation < tolerance.
 */
inline bool isMisorientationMatch(int64 neighborSliceIdx, const std::vector<float32>& neighborQuats, const std::vector<int32>& neighborPhases, int32 curPhase, uint32 laueClass,
                                  const ebsdlib::QuatD& quat1, float32 misorientationTolerance, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps)
{
  const int32 neighborPhase = neighborPhases[neighborSliceIdx];
  if(curPhase != neighborPhase || curPhase <= 0)
  {
    return false;
  }
  const int64 nqOffset = neighborSliceIdx * 4;
  ebsdlib::QuatD quat2(neighborQuats[nqOffset], neighborQuats[nqOffset + 1], neighborQuats[nqOffset + 2], neighborQuats[nqOffset + 3]);
  quat2.positiveOrientation();
  ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass]->calculateMisorientation(quat1, quat2);
  return axisAngle[3] < misorientationTolerance;
}

/**
 * @brief Counts matching good face-neighbors for a bad voxel at (xIdx, yIdx)
 * within the current Z-slice, using 3-slice rolling window buffers.
 * Returns the number of good neighbors with matching orientation.
 */
inline int32 countMatchingNeighbors(int64 xIdx, int64 yIdx, int64 zIdx, int64 dimX, int64 dimY, int64 dimZ, int64 sliceIndex, const std::vector<float32>& prevQuats,
                                    const std::vector<float32>& curQuats, const std::vector<float32>& nextQuats, const std::vector<int32>& prevPhases, const std::vector<int32>& curPhases,
                                    const std::vector<int32>& nextPhases, const std::vector<uint8>& prevMask, const std::vector<uint8>& curMask, const std::vector<uint8>& nextMask, int32 curPhase,
                                    uint32 laueClass, const ebsdlib::QuatD& quat1, float32 misorientationTolerance, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps)
{
  int32 count = 0;
  if(xIdx > 0 && curMask[sliceIndex - 1] && isMisorientationMatch(sliceIndex - 1, curQuats, curPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  if(xIdx < dimX - 1 && curMask[sliceIndex + 1] && isMisorientationMatch(sliceIndex + 1, curQuats, curPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  if(yIdx > 0 && curMask[sliceIndex - dimX] && isMisorientationMatch(sliceIndex - dimX, curQuats, curPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  if(yIdx < dimY - 1 && curMask[sliceIndex + dimX] && isMisorientationMatch(sliceIndex + dimX, curQuats, curPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  if(zIdx > 0 && prevMask[sliceIndex] && isMisorientationMatch(sliceIndex, prevQuats, prevPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  if(zIdx < dimZ - 1 && nextMask[sliceIndex] && isMisorientationMatch(sliceIndex, nextQuats, nextPhases, curPhase, laueClass, quat1, misorientationTolerance, orientationOps))
  {
    count++;
  }
  return count;
}
} // namespace

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
/**
 * @brief Flips bad voxels to good using Z-slice rolling window bulk I/O.
 *
 * Zero O(N) memory: no per-voxel neighborCount or mask arrays.  Instead,
 * neighbor counts are recomputed on-the-fly for each bad voxel during every
 * pass using only O(slice) rolling window buffers.  The mask is read/written
 * per-slice from the real OOC-backed mask array.
 *
 * For each level (6 down to NumberOfNeighbors), repeatedly scan the volume.
 * For each bad voxel, recompute the count of matching good face-neighbors.
 * If count >= currentLevel, flip the voxel.  Repeat until no flips occur.
 */
Result<> BadDataNeighborOrientationCheckScanline::operator()()
{
  const float32 misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float32> / 180.0f;

  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    return MakeErrorResult(-54900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
  }

  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);
  const int64 xyStride = dimX * dimY;
  const usize sliceSize = static_cast<usize>(dimY) * static_cast<usize>(dimX);
  const usize quatSliceElems = sliceSize * 4;

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Cache ensemble-level CrystalStructures locally (tiny array)
  const usize numCrystalStructures = crystalStructures.getNumberOfTuples();
  std::vector<uint32> localCrystalStructures(numCrystalStructures);
  {
    const auto& csStore = crystalStructures.getDataStoreRef();
    csStore.copyIntoBuffer(0, nonstd::span<uint32>(localCrystalStructures.data(), numCrystalStructures));
  }

  auto& quatsStore = quats.getDataStoreRef();
  const auto& phasesStore = cellPhases.getDataStoreRef();

  // Get the mask DataStore for bulk slice reads and per-element flip writes
  auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
  const bool maskIsUInt8 = (maskArray.getDataType() == DataType::uint8);
  AbstractDataStore<uint8>* maskStorePtr = nullptr;
  if(maskIsUInt8)
  {
    maskStorePtr = &dynamic_cast<UInt8Array&>(maskArray).getDataStoreRef();
  }

  // Rolling window buffers — O(slice) memory only
  std::vector<float32> prevQuats(quatSliceElems);
  std::vector<float32> curQuats(quatSliceElems);
  std::vector<float32> nextQuats(quatSliceElems);
  std::vector<int32> prevPhases(sliceSize);
  std::vector<int32> curPhases(sliceSize);
  std::vector<int32> nextPhases(sliceSize);
  std::vector<uint8> prevMask(sliceSize);
  std::vector<uint8> curMask(sliceSize);
  std::vector<uint8> nextMask(sliceSize);

  // Helper to load a mask slice from the store
  auto loadMaskSlice = [&](usize offset, std::vector<uint8>& dest) {
    if(maskStorePtr != nullptr)
    {
      maskStorePtr->copyIntoBuffer(offset, nonstd::span<uint8>(dest.data(), sliceSize));
    }
    else
    {
      // Bool mask: read per-element via maskCompare
      for(usize i = 0; i < sliceSize; i++)
      {
        dest[i] = maskCompare->isTrue(offset + i) ? 1 : 0;
      }
    }
  };

  // Helper to load all 3 arrays for a given Z-slice into dest buffers
  auto loadSlice = [&](int64 z, std::vector<float32>& dstQuats, std::vector<int32>& dstPhases, std::vector<uint8>& dstMask) {
    const usize offset = static_cast<usize>(z) * sliceSize;
    quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(dstQuats.data(), quatSliceElems));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(dstPhases.data(), sliceSize));
    loadMaskSlice(offset, dstMask);
  };

  // Multi-level iterative flipping with on-the-fly neighbor count recomputation.
  // No precomputed neighborCount array — counts are recomputed per voxel per pass.
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

      // Load initial slices for this pass
      loadSlice(0, curQuats, curPhases, curMask);
      if(dimZ > 1)
      {
        loadSlice(1, nextQuats, nextPhases, nextMask);
      }

      for(int64 zIdx = 0; zIdx < dimZ; zIdx++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }

        bool sliceChanged = false;
        for(int64 yIdx = 0; yIdx < dimY; yIdx++)
        {
          const int64 jStride = yIdx * dimX;
          for(int64 xIdx = 0; xIdx < dimX; xIdx++)
          {
            const int64 sliceIndex = jStride + xIdx;

            if(curMask[sliceIndex])
            {
              continue; // already good
            }

            const int64 quatOffset = sliceIndex * 4;
            ebsdlib::QuatD quat1(curQuats[quatOffset], curQuats[quatOffset + 1], curQuats[quatOffset + 2], curQuats[quatOffset + 3]);
            quat1.positiveOrientation();
            const int32 curPhase = curPhases[sliceIndex];
            const uint32 laueClass = localCrystalStructures[curPhase];

            int32 count = countMatchingNeighbors(xIdx, yIdx, zIdx, dimX, dimY, dimZ, sliceIndex, prevQuats, curQuats, nextQuats, prevPhases, curPhases, nextPhases, prevMask, curMask, nextMask,
                                                 curPhase, laueClass, quat1, misorientationTolerance, orientationOps);

            if(count >= currentLevel)
            {
              // Flip this voxel in the local mask buffer (takes effect for
              // subsequent voxels in this slice and as prevMask for the next slice)
              curMask[sliceIndex] = 1;
              sliceChanged = true;
            }
          }
        }

        // Write back any mask changes for this Z-slice to the real store
        if(sliceChanged)
        {
          changed = true;
          const usize sliceOffset = static_cast<usize>(zIdx) * sliceSize;
          if(maskStorePtr != nullptr)
          {
            maskStorePtr->copyFromBuffer(sliceOffset, nonstd::span<const uint8>(curMask.data(), sliceSize));
          }
          else
          {
            for(usize i = 0; i < sliceSize; i++)
            {
              maskCompare->setValue(sliceOffset + i, curMask[i] != 0);
            }
          }
        }

        // Shift rolling window
        std::swap(prevQuats, curQuats);
        std::swap(curQuats, nextQuats);
        std::swap(prevPhases, curPhases);
        std::swap(curPhases, nextPhases);
        std::swap(prevMask, curMask);
        std::swap(curMask, nextMask);
        if(zIdx + 2 < dimZ)
        {
          loadSlice(zIdx + 2, nextQuats, nextPhases, nextMask);
        }
      }
    }
  }

  return {};
}

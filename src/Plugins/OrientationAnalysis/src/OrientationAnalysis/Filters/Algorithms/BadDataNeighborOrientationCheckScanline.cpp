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
 * @brief Checks whether a single face-neighbor has matching orientation.
 *
 * Given a neighbor's slice-local index within one of the rolling window buffers,
 * this function checks:
 *   1. Same phase as the target voxel (and phase > 0, i.e., not unindexed).
 *   2. Misorientation between the target quaternion (quat1) and the neighbor's
 *      quaternion is below the tolerance threshold.
 *
 * The misorientation is computed using the Laue-class-specific symmetry operators
 * via LaueOps::calculateMisorientation(), which returns the minimum misorientation
 * angle across all symmetrically-equivalent representations.
 *
 * @param neighborSliceIdx Index of the neighbor within the slice buffer (not a global voxel index).
 * @param neighborQuats Quaternion buffer for the slice containing the neighbor (4 components per tuple).
 * @param neighborPhases Phase buffer for the slice containing the neighbor.
 * @param curPhase Phase ID of the target (bad) voxel.
 * @param laueClass Crystal structure enum for the target voxel's phase.
 * @param quat1 Quaternion of the target (bad) voxel, already in positive orientation.
 * @param misorientationTolerance Maximum allowed misorientation in radians.
 * @param orientationOps Vector of all LaueOps instances, indexed by crystal structure enum.
 * @return true if the neighbor is same-phase with misorientation below tolerance.
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
 * @brief Counts good face-neighbors with matching orientation for a bad voxel.
 *
 * Examines all 6 face-neighbors of the voxel at position (xIdx, yIdx, zIdx):
 *   - -X, +X, -Y, +Y neighbors are looked up in the current slice buffers (curQuats, curPhases, curMask).
 *   - -Z neighbor is looked up in the previous slice buffers (prevQuats, prevPhases, prevMask).
 *   - +Z neighbor is looked up in the next slice buffers (nextQuats, nextPhases, nextMask).
 *
 * A neighbor "matches" if it is (a) marked as good in the mask, (b) same phase as the
 * target, and (c) within the misorientation tolerance. Boundary checks prevent out-of-bounds
 * access at volume edges.
 *
 * @param xIdx X coordinate of the target voxel within the slice.
 * @param yIdx Y coordinate of the target voxel within the slice.
 * @param zIdx Z coordinate (global) of the target voxel.
 * @param dimX, dimY, dimZ Volume dimensions.
 * @param sliceIndex Linear index within the 2D slice: yIdx * dimX + xIdx.
 * @param prevQuats, curQuats, nextQuats Quaternion rolling window buffers.
 * @param prevPhases, curPhases, nextPhases Phase rolling window buffers.
 * @param prevMask, curMask, nextMask Mask rolling window buffers.
 * @param curPhase Phase ID of the target voxel.
 * @param laueClass Crystal structure enum for the target voxel's phase.
 * @param quat1 Quaternion of the target voxel.
 * @param misorientationTolerance Tolerance in radians.
 * @param orientationOps LaueOps vector.
 * @return Number of matching good face-neighbors (0-6).
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
 * @brief OOC-safe bad-voxel flipping using Z-slice rolling window bulk I/O.
 *
 * This algorithm uses O(3 * sliceSize) memory for a 3-slice rolling window
 * (previous, current, next Z-slices) instead of any global per-voxel arrays.
 * Neighbor counts are recomputed on-the-fly for each bad voxel on every pass,
 * trading computation for strictly sequential I/O that avoids chunk thrashing.
 *
 * **Outer loop** (level from 6 down to NumberOfNeighbors):
 *   At each level, the required neighbor count to flip a voxel decreases by 1.
 *   Starting at 6 (all neighbors must agree) and relaxing to the user's threshold
 *   ensures that high-confidence flips happen first, which in turn enables
 *   additional flips in subsequent levels (cascade effect).
 *
 * **Inner loop** (pass until convergence):
 *   For each pass at a given level:
 *   1. Load Z-slices 0 and 1 into the rolling window.
 *   2. Scan every voxel in the current slice. For each bad voxel, recompute the
 *      count of matching good face-neighbors using the 3-slice window.
 *   3. If count >= currentLevel, flip the voxel's mask in the local buffer.
 *   4. If any flips occurred in the slice, write the updated mask back to the
 *      OOC store and set the "changed" flag to trigger another pass.
 *   5. Shift the rolling window forward by one Z-slice.
 *
 * Passes repeat until no voxels flip in a full volume scan, then the level decrements.
 */
Result<> BadDataNeighborOrientationCheckScanline::operator()()
{
  // Convert misorientation tolerance from degrees to radians for LaueOps comparison.
  const float32 misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float32> / 180.0f;

  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeomPath);
  SizeVec3 udims = imageGeomPtr->getDimensions();
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Instantiate the mask comparison utility, which handles both bool and uint8 mask types.
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
  const usize quatSliceElems = sliceSize * 4; // 4 quaternion components per voxel

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Cache the ensemble-level crystal structures array locally. This tiny array
  // (one entry per phase) is accessed for every neighbor comparison, so caching
  // avoids repeated per-element OOC reads.
  const usize numCrystalStructures = crystalStructures.getNumberOfTuples();
  std::vector<uint32> localCrystalStructures(numCrystalStructures);
  {
    const auto& csStore = crystalStructures.getDataStoreRef();
    csStore.copyIntoBuffer(0, nonstd::span<uint32>(localCrystalStructures.data(), numCrystalStructures));
  }

  // Obtain DataStore references for bulk slice I/O. All per-element access goes
  // through copyIntoBuffer()/copyFromBuffer() to maintain sequential access patterns.
  auto& quatsStore = quats.getDataStoreRef();
  const auto& phasesStore = cellPhases.getDataStoreRef();

  // For uint8 masks, we can use bulk copyIntoBuffer()/copyFromBuffer() directly
  // on the mask store. For bool masks, we fall back to per-element maskCompare
  // access because there is no typed BoolAbstractDataStore with bulk I/O support.
  auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
  const bool maskIsUInt8 = (maskArray.getDataType() == DataType::uint8);
  AbstractDataStore<uint8>* maskStorePtr = nullptr;
  if(maskIsUInt8)
  {
    maskStorePtr = &dynamic_cast<UInt8Array&>(maskArray).getDataStoreRef();
  }

  // ---- Rolling window buffers ----
  // Three Z-slices of quaternions, phases, and mask values. These are swapped
  // (not copied) as the window advances, so the total memory is 3 * sliceSize
  // per array type.
  std::vector<float32> prevQuats(quatSliceElems);
  std::vector<float32> curQuats(quatSliceElems);
  std::vector<float32> nextQuats(quatSliceElems);
  std::vector<int32> prevPhases(sliceSize);
  std::vector<int32> curPhases(sliceSize);
  std::vector<int32> nextPhases(sliceSize);
  std::vector<uint8> prevMask(sliceSize);
  std::vector<uint8> curMask(sliceSize);
  std::vector<uint8> nextMask(sliceSize);

  // Helper to load a mask slice from the store. For uint8 masks, uses bulk
  // copyIntoBuffer() which is efficient for OOC stores. For bool masks, falls
  // back to per-element access via maskCompare because BoolArray lacks typed
  // bulk I/O support. The bool path is slower but only used in rare cases.
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

  // Helper to bulk-load all three per-voxel arrays (quaternions, phases, mask) for a
  // single Z-slice into the destination buffers. Each call issues 3 copyIntoBuffer()
  // calls against the OOC stores, loading one complete Z-slice per array.
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

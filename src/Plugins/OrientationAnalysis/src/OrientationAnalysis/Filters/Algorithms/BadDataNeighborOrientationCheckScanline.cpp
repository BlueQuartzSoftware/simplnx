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
 * @brief Tests whether one neighbor matches the target orientation.
 * @param neighborSliceIdx Identifies the neighbor in its slice buffer.
 * @param neighborQuats Supplies four-component neighbor quaternions.
 * @param neighborPhases Supplies neighbor phase IDs.
 * @param curPhase Identifies the target phase.
 * @param laueClass Identifies the target crystal structure.
 * @param quat1 Supplies the positive-orientation target quaternion.
 * @param misorientationTolerance Specifies the strict radian limit.
 * @param orientationOps Supplies crystal-structure symmetry operators.
 * @return True if phase and strict misorientation tests pass.
 */
inline bool isMisorientationMatch(int64 neighborSliceIdx, const std::vector<float32>& neighborQuats, const std::vector<int32>& neighborPhases, int32 curPhase, uint32 laueClass,
                                  const ebsdlib::QuatD& quat1, float64 misorientationTolerance, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps)
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
 * @brief Counts matching good face neighbors for one bad voxel.
 * @param xIdx Identifies the target X coordinate.
 * @param yIdx Identifies the target Y coordinate.
 * @param zIdx Identifies the target Z coordinate.
 * @param dimX Specifies the X dimension.
 * @param dimY Specifies the Y dimension.
 * @param dimZ Specifies the Z dimension.
 * @param sliceIndex Identifies the target in each slice buffer.
 * @param prevQuats Supplies the previous quaternion slice.
 * @param curQuats Supplies the current quaternion slice.
 * @param nextQuats Supplies the next quaternion slice.
 * @param prevPhases Supplies the previous phase slice.
 * @param curPhases Supplies the current phase slice.
 * @param nextPhases Supplies the next phase slice.
 * @param prevMask Supplies the previous mask slice.
 * @param curMask Supplies the current mask slice.
 * @param nextMask Supplies the next mask slice.
 * @param curPhase Identifies the target phase.
 * @param laueClass Identifies the target crystal structure.
 * @param quat1 Supplies the target quaternion.
 * @param misorientationTolerance Specifies the strict radian limit.
 * @param orientationOps Supplies crystal-structure symmetry operators.
 * @return Matching good face-neighbor count.
 */
inline int32 countMatchingNeighbors(int64 xIdx, int64 yIdx, int64 zIdx, int64 dimX, int64 dimY, int64 dimZ, int64 sliceIndex, const std::vector<float32>& prevQuats,
                                    const std::vector<float32>& curQuats, const std::vector<float32>& nextQuats, const std::vector<int32>& prevPhases, const std::vector<int32>& curPhases,
                                    const std::vector<int32>& nextPhases, const std::vector<uint8>& prevMask, const std::vector<uint8>& curMask, const std::vector<uint8>& nextMask, int32 curPhase,
                                    uint32 laueClass, const ebsdlib::QuatD& quat1, float64 misorientationTolerance, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps)
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
Result<> BadDataNeighborOrientationCheckScanline::operator()()
{
  // Double-precision pi keeps the strict comparison aligned with boundary-exact analytical fixtures.
  const float64 misorientationTolerance = static_cast<float64>(m_InputValues->MisorientationTolerance) * numbers::pi_v<float64> / 180.0;

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
    // Direct callers can bypass preflight, so mask type errors return a Result.
    return MakeErrorResult(-54900,
                           fmt::format("Mask Array at '{}' could not be loaded; expected Bool or UInt8 backing. Underlying error: {}", m_InputValues->MaskArrayPath.toString(), exception.what()));
  }

  const int64 dimX = static_cast<int64>(udims[0]);
  const int64 dimY = static_cast<int64>(udims[1]);
  const int64 dimZ = static_cast<int64>(udims[2]);
  const int64 xyStride = dimX * dimY;
  const usize sliceSize = static_cast<usize>(dimY) * static_cast<usize>(dimX);
  const usize quatSliceElems = sliceSize * 4;

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Cache the small ensemble array because each neighbor comparison needs its crystal structure.
  const usize numCrystalStructures = crystalStructures.getNumberOfTuples();
  std::vector<uint32> localCrystalStructures(numCrystalStructures);
  {
    const auto& csStore = crystalStructures.getDataStoreRef();
    csStore.copyIntoBuffer(0, nonstd::span<uint32>(localCrystalStructures.data(), numCrystalStructures));
  }

  // Validate indexes before orientationOps access. UnknownCrystalStructure remains a supported sentinel.
  const usize numOrientationOps = orientationOps.size();
  for(usize i = 0; i < numCrystalStructures; ++i)
  {
    if(localCrystalStructures[i] >= numOrientationOps && localCrystalStructures[i] != ebsdlib::CrystalStructure::UnknownCrystalStructure)
    {
      return MakeErrorResult(
          -54901, fmt::format("Crystal structure at ensemble index {} has value {}, which is not a valid Laue-group index. Valid range is [0, {}).", i, localCrystalStructures[i], numOrientationOps));
    }
  }

  // Per-voxel arrays use bulk slice I/O to preserve sequential OOC access.
  auto& quatsStore = quats.getDataStoreRef();
  const auto& phasesStore = cellPhases.getDataStoreRef();

  // The bool path converts one bulk-read scratch slice to uint8 values. This avoids per-element OOC access.
  auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
  const DataType maskDataType = maskArray.getDataType();
  AbstractDataStore<uint8>* maskStorePtr = nullptr;
  AbstractDataStore<bool>* maskStoreBoolPtr = nullptr;
  if(maskDataType == DataType::uint8)
  {
    maskStorePtr = &dynamic_cast<UInt8Array&>(maskArray).getDataStoreRef();
  }
  else if(maskDataType == DataType::boolean)
  {
    maskStoreBoolPtr = &dynamic_cast<BoolArray&>(maskArray).getDataStoreRef();
  }
  // A raw bool buffer supplies a span<bool>; std::vector<bool> cannot supply bool storage.
  std::unique_ptr<bool[]> boolSliceScratch;
  if(maskStoreBoolPtr != nullptr)
  {
    boolSliceScratch = std::make_unique<bool[]>(sliceSize);
  }

  // Swapped slices keep the rolling window bounded without copying prior slice contents.
  std::vector<float32> prevQuats(quatSliceElems);
  std::vector<float32> curQuats(quatSliceElems);
  std::vector<float32> nextQuats(quatSliceElems);
  std::vector<int32> prevPhases(sliceSize);
  std::vector<int32> curPhases(sliceSize);
  std::vector<int32> nextPhases(sliceSize);
  std::vector<uint8> prevMask(sliceSize);
  std::vector<uint8> curMask(sliceSize);
  std::vector<uint8> nextMask(sliceSize);

  auto loadMaskSlice = [&](usize offset, std::vector<uint8>& dest) {
    if(maskStorePtr != nullptr)
    {
      maskStorePtr->copyIntoBuffer(offset, nonstd::span<uint8>(dest.data(), sliceSize));
    }
    else if(maskStoreBoolPtr != nullptr)
    {
      maskStoreBoolPtr->copyIntoBuffer(offset, nonstd::span<bool>(boolSliceScratch.get(), sliceSize));
      for(usize i = 0; i < sliceSize; i++)
      {
        dest[i] = boolSliceScratch[i] ? 1 : 0;
      }
    }
    else
    {
      for(usize i = 0; i < sliceSize; i++)
      {
        dest[i] = maskCompare->isTrue(offset + i) ? 1 : 0;
      }
    }
  };

  // Each load issues one bulk read for quaternions, phases, and mask values.
  auto loadSlice = [&](int64 z, std::vector<float32>& dstQuats, std::vector<int32>& dstPhases, std::vector<uint8>& dstMask) {
    const usize offset = static_cast<usize>(z) * sliceSize;
    quatsStore.copyIntoBuffer(offset * 4, nonstd::span<float32>(dstQuats.data(), quatSliceElems));
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(dstPhases.data(), sliceSize));
    loadMaskSlice(offset, dstMask);
  };

  // Recompute counts per pass to avoid a global random-write neighbor-count array.
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
            // UnknownCrystalStructure has no LaueOps entry and cannot participate in a match.
            if(laueClass >= numOrientationOps)
            {
              continue;
            }

            int32 count = countMatchingNeighbors(xIdx, yIdx, zIdx, dimX, dimY, dimZ, sliceIndex, prevQuats, curQuats, nextQuats, prevPhases, curPhases, nextPhases, prevMask, curMask, nextMask,
                                                 curPhase, laueClass, quat1, misorientationTolerance, orientationOps);

            if(count >= currentLevel)
            {
              // Local changes affect later voxels in this slice and the next slice's previous mask.
              curMask[sliceIndex] = 1;
              sliceChanged = true;
            }
          }
        }

        if(sliceChanged)
        {
          changed = true;
          const usize sliceOffset = static_cast<usize>(zIdx) * sliceSize;
          if(maskStorePtr != nullptr)
          {
            maskStorePtr->copyFromBuffer(sliceOffset, nonstd::span<const uint8>(curMask.data(), sliceSize));
          }
          else if(maskStoreBoolPtr != nullptr)
          {
            for(usize i = 0; i < sliceSize; i++)
            {
              boolSliceScratch[i] = curMask[i] != 0;
            }
            maskStoreBoolPtr->copyFromBuffer(sliceOffset, nonstd::span<const bool>(boolSliceScratch.get(), sliceSize));
          }
          else
          {
            for(usize i = 0; i < sliceSize; i++)
            {
              maskCompare->setValue(sliceOffset + i, curMask[i] != 0);
            }
          }
        }

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

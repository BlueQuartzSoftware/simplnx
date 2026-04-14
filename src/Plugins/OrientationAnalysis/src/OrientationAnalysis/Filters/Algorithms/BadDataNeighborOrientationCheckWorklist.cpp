#include "BadDataNeighborOrientationCheckWorklist.hpp"

#include "BadDataNeighborOrientationCheck.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <deque>

using namespace nx::core;

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckWorklist::BadDataNeighborOrientationCheckWorklist(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                 const BadDataNeighborOrientationCheckInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
BadDataNeighborOrientationCheckWorklist::~BadDataNeighborOrientationCheckWorklist() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief In-core bad-voxel flipping using two-phase worklist propagation.
 *
 * This algorithm exploits random-access O(1) DataArray subscript access (safe for
 * in-core stores) to achieve O(flipped) amortized cost instead of the O(N * passes)
 * cost of the Scanline variant's full-volume rescans.
 *
 * **Phase 1 -- Initial neighbor counting** (single linear scan, O(N)):
 *   For every bad voxel, iterate over its 6 face-neighbors. For each good neighbor
 *   with the same phase and misorientation within tolerance, increment the voxel's
 *   neighborCount. This produces a baseline count before any flips occur.
 *
 * **Phase 2 -- Worklist-driven propagation** (per level, O(flipped)):
 *   For each level (6 down to NumberOfNeighbors):
 *     1. Seed a deque with all bad voxels whose neighborCount >= currentLevel.
 *     2. Pop the front voxel. If it has already been flipped (by a neighbor cascade)
 *        or its count has dropped below the threshold (impossible in practice but
 *        checked defensively), skip it.
 *     3. Flip the voxel's mask to true.
 *     4. For each still-bad face-neighbor of the newly-flipped voxel: check if the
 *        neighbor has a matching orientation (same phase, misorientation < tolerance).
 *        If so, increment its neighborCount. If the count now meets the threshold,
 *        enqueue the neighbor for processing.
 *     5. Repeat until the deque drains, then move to the next level.
 *
 * This is essentially a breadth-first flood-fill constrained by crystallographic
 * misorientation. The cascade effect means that flipping one voxel can immediately
 * enable its neighbors to flip, propagating outward from high-confidence seeds.
 *
 * **Why this is not suitable for OOC**: The deque pops voxels in arbitrary spatial
 * order (BFS wavefront). Each pop accesses the popped voxel's quaternion, phase,
 * and mask, plus all 6 neighbors' data -- all random-access lookups. On OOC stores,
 * each such lookup could trigger a disk-chunk load/evict, creating catastrophic
 * chunk thrashing for large datasets.
 */
Result<> BadDataNeighborOrientationCheckWorklist::operator()()
{
  // Convert misorientation tolerance from degrees to radians.
  const float32 misorientationTolerance = m_InputValues->MisorientationTolerance * numbers::pi_v<float32> / 180.0f;

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

  // Precompute the 6 face-neighbor index offsets (-X, +X, -Y, +Y, -Z, +Z) relative
  // to a voxel's linear index in the flat array. These are constant for any given
  // volume geometry.
  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Per-voxel count of good face-neighbors with matching orientation. This O(N) array
  // is the trade-off: the Worklist variant uses O(N) memory to achieve O(flipped)
  // propagation speed, while the Scanline variant uses O(slice) memory but O(N * passes).
  std::vector<int32> neighborCount(totalPoints, 0);

  // ===== Phase 1: Count matching good neighbors for each bad voxel =====
  // Single linear scan over all voxels. For each bad voxel, check its 6 face-neighbors
  // for good voxels with matching phase and orientation within tolerance.
  for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    if(!maskCompare->isTrue(voxelIndex))
    {
      // Build the target voxel's quaternion for misorientation comparisons.
      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

      // Decompose the linear index into (x, y, z) coordinates for boundary checks.
      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }
        const int64 neighborPoint = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[faceIndex];

        // Only count good neighbors (mask == true) with the same phase and a
        // misorientation below the tolerance.
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

  // ===== Phase 2: Iteratively flip bad voxels using worklist =====
  // Iterate from the strictest level (6 = all face-neighbors must agree) down to
  // the user's minimum. At each level, seed the worklist with all eligible voxels,
  // then drain it with propagation.
  constexpr int32 startLevel = 6;
  const int32 totalLevels = startLevel - m_InputValues->NumberOfNeighbors + 1;

  for(int32 currentLevel = startLevel; currentLevel >= m_InputValues->NumberOfNeighbors; currentLevel--)
  {
    // Seed the worklist with all bad voxels that already meet this level's threshold.
    std::deque<usize> worklist;
    for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      if(neighborCount[voxelIndex] >= currentLevel && !maskCompare->isTrue(voxelIndex))
      {
        worklist.push_back(voxelIndex);
      }
    }

    // Process the worklist. When a voxel is flipped, its still-bad neighbors may
    // gain a new matching good neighbor and become eligible, creating a cascade.
    while(!worklist.empty())
    {
      const usize voxelIndex = worklist.front();
      worklist.pop_front();

      // Defensive check: skip if already flipped (by a prior cascade) or if the
      // count dropped below threshold (should not happen, but guards correctness).
      if(maskCompare->isTrue(voxelIndex) || neighborCount[voxelIndex] < currentLevel)
      {
        continue;
      }

      // Flip this voxel from bad to good.
      maskCompare->setValue(voxelIndex, true);

      // Now propagate: for each still-bad face-neighbor, check if the newly-flipped
      // voxel constitutes a new matching good neighbor for that neighbor.
      ebsdlib::QuatD quat1(quats[voxelIndex * 4], quats[voxelIndex * 4 + 1], quats[voxelIndex * 4 + 2], quats[voxelIndex * 4 + 3]);
      quat1.positiveOrientation();
      const uint32 laueClass1 = crystalStructures[cellPhases[voxelIndex]];

      const int64 xIdx = static_cast<int64>(voxelIndex) % dims[0];
      const int64 yIdx = (static_cast<int64>(voxelIndex) / dims[0]) % dims[1];
      const int64 zIdx = static_cast<int64>(voxelIndex) / xyStride;

      std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }

        const int64 neighborPoint = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[faceIndex];

        if(!maskCompare->isTrue(neighborPoint))
        {
          if(cellPhases[voxelIndex] == cellPhases[neighborPoint] && cellPhases[voxelIndex] > 0)
          {
            ebsdlib::QuatD quat2(quats[neighborPoint * 4], quats[neighborPoint * 4 + 1], quats[neighborPoint * 4 + 2], quats[neighborPoint * 4 + 3]);
            quat2.positiveOrientation();
            ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
            if(axisAngle[3] < misorientationTolerance)
            {
              // Increment the neighbor's count because the just-flipped voxel is
              // now a new good neighbor for it.
              neighborCount[neighborPoint]++;
              // If the neighbor now meets the threshold, enqueue it for processing.
              // It may be enqueued multiple times as different neighbors flip, but
              // the defensive check at the top of the while loop handles duplicates.
              if(neighborCount[neighborPoint] >= currentLevel)
              {
                worklist.push_back(static_cast<usize>(neighborPoint));
              }
            }
          }
        }
      }
    }
  }

  return {};
}

#include "EBSDSegmentFeatures.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <algorithm>

using namespace nx::core;

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
  m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  m_IsPeriodic = inputValues->IsPeriodic;
}

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::~EBSDSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
// Segments an EBSD dataset into crystallographic features (grains) by flood-
// filling contiguous voxels whose crystal orientations are within a user-
// specified misorientation tolerance. Two voxels are grouped into the same
// feature only if they share the same phase and their misorientation (computed
// via the appropriate LaueOps symmetry operator) is below the threshold.
//
// Algorithm dispatch:
//   - In-core data  -> execute()    : classic depth-first-search (DFS) flood fill
//   - Out-of-core   -> executeCCL() : connected-component labeling that streams
//                                     data slice-by-slice to limit memory usage
//   The choice is made by checking IsOutOfCore() on the FeatureIds array (i.e.,
//   whether the backing DataStore lives on disk) or if ForceOocAlgorithm() is
//   set (used for testing).
//
// Post-processing after either algorithm:
//   1. Validate that at least one feature was found (error if not).
//   2. Resize the Feature AttributeMatrix to (m_FoundFeatures + 1) tuples so
//      that all per-feature arrays (Active, etc.) have the correct size.
//      Index 0 is reserved as an invalid/background feature.
//   3. Initialize the Active array: fill with 1 (active), then set index 0
//      to 0 to mark it as the reserved background slot.
//   4. Optionally randomize FeatureIds so that spatially adjacent grains get
//      non-sequential IDs, improving visual contrast in color-mapped renders.
// -----------------------------------------------------------------------------
Result<> EBSDSegmentFeatures::operator()()
{
  this->m_NeighborScheme = m_InputValues->NeighborScheme;
  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-485090, message);
    }
  }
  m_CrystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0); // initialize the output array with zeros

  // Dispatch between DFS (in-core) and CCL (OOC) algorithms
  if(IsOutOfCore(*m_FeatureIdsArray) || ForceOocAlgorithm())
  {
    SizeVec3 udims = gridGeom->getDimensions();
    allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

    auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
    executeCCL(gridGeom, featureIdsStore);

    deallocateSliceBuffers();
  }
  else
  {
    execute(gridGeom);
  }
  // Sanity check the result.
  if(this->m_FoundFeatures < 1)
  {
    return MakeErrorResult(-87000, fmt::format("The number of Features is '{}' which means no Features were detected. A threshold value may be set incorrectly", this->m_FoundFeatures));
  }

  // Resize the Feature Attribute Matrix
  ShapeType tDims = {static_cast<usize>(this->m_FoundFeatures + 1)};
  auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  cellFeaturesAM.resizeTuples(tDims); // This will resize the active array

  // make sure all values are initialized and "re-reserve" index 0
  auto* activeArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  activeArray->getDataStore()->fill(1);
  (*activeArray)[0] = 0;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  if(m_InputValues->RandomizeFeatureIds)
  {
    randomizeFeatureIds(m_FeatureIdsArray, this->m_FoundFeatures + 1);
  }

  return {};
}

// -----------------------------------------------------------------------------
// Finds the next unassigned voxel that can serve as the seed for a new feature.
// The scan is a simple linear walk starting from `nextSeed`, which is the index
// immediately after the last seed found. This avoids rescanning already-assigned
// voxels at the beginning of the array.
//
// A voxel is eligible to become a seed when all three conditions are met:
//   1. featureId == 0 : the voxel has not yet been assigned to any feature.
//   2. Passes the mask: if masking is enabled, the voxel must be flagged as
//      "good" (e.g., not a bad scan point).
//   3. cellPhase > 0  : the voxel belongs to a real crystallographic phase
//      (phase 0 is reserved for unindexed/background points).
//
// When a valid seed is found, its featureId is immediately set to `gnum`
// (the new feature number) so that subsequent calls will skip it.
// Returns the linear index of the seed, or -1 if no more seeds exist.
// -----------------------------------------------------------------------------
int64_t EBSDSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  nx::core::DataArray<int32>::store_type* featureIds = m_FeatureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  nx::core::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int64 seed = -1;
  // start with the next voxel after the last seed
  auto randPoint = static_cast<usize>(nextSeed);
  while(seed == -1 && randPoint < totalPoints)
  {
    if(featureIds->getValue(randPoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if((!m_InputValues->UseMask || m_GoodVoxelsArray->isTrue(randPoint)) && cellPhases->getValue(randPoint) > 0)
      {
        seed = static_cast<int64>(randPoint);
      }
      else
      {
        randPoint += 1;
      }
    }
    else
    {
      randPoint += 1;
    }
  }
  if(seed >= 0)
  {
    featureIds->setValue(static_cast<usize>(seed), gnum);
  }
  return seed;
}

// -----------------------------------------------------------------------------
// Determines whether a neighboring voxel should be merged into the current
// feature during the DFS flood fill (execute() path). This is NOT used by
// the CCL path, which calls areNeighborsSimilar() instead.
//
// The method checks three conditions before grouping:
//   1. The neighbor's featureId must be 0 (unassigned).
//   2. The neighbor must pass the mask (if masking is enabled).
//   3. The neighbor must be crystallographically similar to the reference voxel.
//
// Similarity check (misorientation):
//   - Look up the Laue class for both voxels from their phase -> crystal
//     structure mapping. If either Laue class is out of range (>= number of
//     known symmetry operators, e.g., phase == 999), bail out immediately.
//   - Extract the quaternion orientations (4 floats per voxel) for both points.
//   - If both voxels share the same phase, compute the misorientation angle via
//     LaueOps::calculateMisorientation(), which returns an axis-angle pair.
//     The angle (w, in radians) accounts for crystal symmetry equivalences.
//   - If w < MisorientationTolerance, the voxels are considered part of the
//     same grain. The neighbor's featureId is set to `gnum` as a side effect.
// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const
{
  bool group = false;

  // Get the phases for each voxel
  nx::core::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int32_t laueClass1 = (*m_CrystalStructures)[(*cellPhases)[referencePoint]];
  int32_t laueClass2 = (*m_CrystalStructures)[(*cellPhases)[neighborPoint]];
  // If either of the phases is 999 then we bail out now.
  if(laueClass1 >= m_OrientationOps.size() || laueClass2 >= m_OrientationOps.size())
  {
    return group;
  }
  Float32Array& currentQuatPtr = *m_QuatsArray;
  Int32Array& featureIds = *m_FeatureIdsArray;

  bool neighborPointIsGood = false;
  if(m_GoodVoxelsArray != nullptr)
  {
    neighborPointIsGood = m_GoodVoxelsArray->isTrue(neighborPoint);
  }

  if(featureIds[neighborPoint] == 0 && (m_GoodVoxelsArray == nullptr || neighborPointIsGood))
  {
    float w = std::numeric_limits<float>::max();
    const ebsdlib::QuatD q1(currentQuatPtr[referencePoint * 4], currentQuatPtr[referencePoint * 4 + 1], currentQuatPtr[referencePoint * 4 + 2], currentQuatPtr[referencePoint * 4 + 3]);
    const ebsdlib::QuatD q2(currentQuatPtr[neighborPoint * 4], currentQuatPtr[neighborPoint * 4 + 1], currentQuatPtr[neighborPoint * 4 + 2], currentQuatPtr[neighborPoint * 4 + 3]);

    if((*cellPhases)[referencePoint] == (*cellPhases)[neighborPoint])
    {
      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass1]->calculateMisorientation(q1, q2);
      w = static_cast<float>(axisAngle[3]);
    }
    if(w < m_InputValues->MisorientationTolerance)
    {
      group = true;
      featureIds[neighborPoint] = gnum;
    }
  }

  return group;
}

// -----------------------------------------------------------------------------
// Checks whether a single voxel is eligible for segmentation (used by the CCL
// path in executeCCL()). A voxel is valid if it passes the mask and has a
// crystallographic phase > 0.
//
// Slice buffer fast path:
//   When m_UseSliceBuffers is true (OOC mode), the method first checks whether
//   the voxel's Z-slice is currently loaded in the rolling 2-slot buffer. The
//   slot is determined by (iz % 2). If the voxel's slice matches the buffered
//   slice index, the mask and phase values are read directly from the in-memory
//   m_MaskBuffer and m_PhaseBuffer arrays, avoiding an on-disk I/O round-trip.
//
// OOC fallback:
//   If slice buffers are not active, or if the voxel's slice is not currently
//   buffered (which can happen during Phase 1b of CCL when periodic boundary
//   merging accesses non-adjacent slices), the method falls back to direct
//   array access through the DataStore, which may trigger on-disk I/O for
//   out-of-core data.
// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz = point / m_BufSliceSize;
    const int slot = static_cast<int>(iz % 2);
    if(m_BufferedSliceZ[slot] == iz)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off = static_cast<usize>(slot) * sliceSize + static_cast<usize>(point - iz * m_BufSliceSize);
      if(m_InputValues->UseMask && m_MaskBuffer[off] == 0)
      {
        return false;
      }
      if(m_PhaseBuffer[off] <= 0)
      {
        return false;
      }
      return true;
    }
  }

  // OOC fallback
  if(m_InputValues->UseMask && !m_GoodVoxelsArray->isTrue(point))
  {
    return false;
  }
  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();
  if(cellPhases[point] <= 0)
  {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Determines whether two neighboring voxels are crystallographically similar
// enough to belong to the same feature. Used exclusively by the CCL path
// (executeCCL()), whereas the DFS path uses determineGrouping() instead.
//
// Slice buffer fast path:
//   When both voxels' Z-slices are present in the rolling 2-slot buffer, all
//   data is read from the in-memory buffers (m_QuatBuffer, m_PhaseBuffer,
//   m_MaskBuffer). The buffer offset for each point is computed as:
//     slot * sliceSize + (point - iz * sliceSize)
//   For quaternions, an additional x4 factor accounts for the 4 components
//   per voxel. The method then:
//     1. Checks point2's mask validity.
//     2. Checks that point2's phase > 0 and both phases match.
//     3. Looks up the Laue class and verifies it is in range.
//     4. Constructs QuatD objects from the buffered quaternion components.
//     5. Computes misorientation via LaueOps::calculateMisorientation().
//     6. Returns true if the misorientation angle < MisorientationTolerance.
//
// OOC fallback:
//   If either voxel's slice is not buffered (e.g., during Phase 1b periodic
//   merge), falls back to direct DataStore access: validates point2 via
//   isValidVoxel(), checks phase equality, then computes misorientation from
//   the full quaternion and phase arrays on disk.
// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz1 = point1 / m_BufSliceSize;
    const int slot1 = static_cast<int>(iz1 % 2);
    const int64 iz2 = point2 / m_BufSliceSize;
    const int slot2 = static_cast<int>(iz2 % 2);

    if(m_BufferedSliceZ[slot1] == iz1 && m_BufferedSliceZ[slot2] == iz2)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off1 = static_cast<usize>(slot1) * sliceSize + static_cast<usize>(point1 - iz1 * m_BufSliceSize);
      const usize off2 = static_cast<usize>(slot2) * sliceSize + static_cast<usize>(point2 - iz2 * m_BufSliceSize);

      // Check point2 validity
      if(m_InputValues->UseMask && m_MaskBuffer[off2] == 0)
      {
        return false;
      }
      const int32 phase1 = m_PhaseBuffer[off1];
      const int32 phase2 = m_PhaseBuffer[off2];
      if(phase2 <= 0)
      {
        return false;
      }
      if(phase1 != phase2)
      {
        return false;
      }

      int32 laueClass = (*m_CrystalStructures)[phase1];
      if(static_cast<usize>(laueClass) >= m_OrientationOps.size())
      {
        return false;
      }

      const usize q1Base = static_cast<usize>(slot1) * sliceSize * 4 + static_cast<usize>(point1 - iz1 * m_BufSliceSize) * 4;
      const usize q2Base = static_cast<usize>(slot2) * sliceSize * 4 + static_cast<usize>(point2 - iz2 * m_BufSliceSize) * 4;

      const ebsdlib::QuatD q1(m_QuatBuffer[q1Base], m_QuatBuffer[q1Base + 1], m_QuatBuffer[q1Base + 2], m_QuatBuffer[q1Base + 3]);
      const ebsdlib::QuatD q2(m_QuatBuffer[q2Base], m_QuatBuffer[q2Base + 1], m_QuatBuffer[q2Base + 2], m_QuatBuffer[q2Base + 3]);

      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
      float w = static_cast<float>(axisAngle[3]);

      return w < m_InputValues->MisorientationTolerance;
    }
  }

  // OOC fallback (original code)
  if(!isValidVoxel(point2))
  {
    return false;
  }

  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();

  if(cellPhases[point1] != cellPhases[point2])
  {
    return false;
  }

  int32 laueClass = (*m_CrystalStructures)[cellPhases[point1]];
  if(static_cast<usize>(laueClass) >= m_OrientationOps.size())
  {
    return false;
  }

  Float32Array& quats = *m_QuatsArray;
  const ebsdlib::QuatD q1(quats[point1 * 4], quats[point1 * 4 + 1], quats[point1 * 4 + 2], quats[point1 * 4 + 3]);
  const ebsdlib::QuatD q2(quats[point2 * 4], quats[point2 * 4 + 1], quats[point2 * 4 + 2], quats[point2 * 4 + 3]);

  ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
  float w = static_cast<float>(axisAngle[3]);

  return w < m_InputValues->MisorientationTolerance;
}

// -----------------------------------------------------------------------------
// Allocates the rolling 2-slot slice buffers used by the CCL (OOC) algorithm.
// Called once at the start of the OOC branch in operator(), before executeCCL().
//
// Each slot holds one full XY slice (dimX * dimY voxels). Two slots are needed
// because the CCL algorithm compares the current slice (iz) with the previous
// slice (iz-1), so both must be in memory simultaneously.
//
// Buffers allocated:
//   - m_QuatBuffer  : 2 * sliceSize * 4 floats  (quaternion: 4 components/voxel)
//   - m_PhaseBuffer : 2 * sliceSize int32 values (one phase ID per voxel)
//   - m_MaskBuffer  : 2 * sliceSize uint8 values (one mask flag per voxel)
//
// Both m_BufferedSliceZ slots are initialized to -1 (no slice loaded).
// m_UseSliceBuffers is set to true so that isValidVoxel() and
// areNeighborsSimilar() will use the fast buffer path.
// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  m_QuatBuffer.resize(2 * sliceSize * 4);
  m_PhaseBuffer.resize(2 * sliceSize);
  m_MaskBuffer.resize(2 * sliceSize);
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_UseSliceBuffers = true;
}

// -----------------------------------------------------------------------------
// Releases the slice buffers after executeCCL() completes, freeing the memory
// back to the system. Called in the OOC branch of operator() after the CCL
// algorithm finishes. Resets m_UseSliceBuffers to false and both
// m_BufferedSliceZ slots to -1. The vectors are replaced with default-
// constructed (empty) instances to guarantee memory deallocation.
// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_QuatBuffer = std::vector<float32>();
  m_PhaseBuffer = std::vector<int32>();
  m_MaskBuffer = std::vector<uint8>();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
}

// -----------------------------------------------------------------------------
// Pre-loads voxel data for a single Z-slice into the rolling 2-slot buffer,
// called by executeCCL() before processing each slice.
//
// Rolling buffer design:
//   The target slot is determined by (iz % 2), so even slices go to slot 0 and
//   odd slices go to slot 1. Because the CCL algorithm processes slices in
//   order (0, 1, 2, ...), at any given slice iz the previous slice (iz-1) is
//   always in the other slot, keeping both the current and previous slice data
//   available in memory.
//
// Sentinel behavior:
//   If iz < 0, slice buffering is disabled (m_UseSliceBuffers = false). The
//   CCL algorithm passes iz = -1 after completing the slice-by-slice sweep to
//   signal that subsequent calls (e.g., during Phase 1b periodic boundary
//   merging) should use direct DataStore access instead of the buffers.
//
// Skip-if-already-loaded:
//   If m_BufferedSliceZ[slot] == iz, the data for this slice is already in the
//   buffer (e.g., from a previous prepareForSlice call), so the method returns
//   immediately without re-reading.
//
// Data loaded per slice:
//   - Quaternions (4 float32 per voxel) into m_QuatBuffer
//   - Phase IDs (1 int32 per voxel) into m_PhaseBuffer
//   - Mask flags (1 uint8 per voxel) into m_MaskBuffer; if masking is disabled,
//     all mask values are set to 1 (valid)
// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return;
  }

  if(!m_UseSliceBuffers)
  {
    return;
  }

  const int slot = static_cast<int>(iz % 2);
  if(m_BufferedSliceZ[slot] == iz)
  {
    return;
  }

  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(slot) * sliceSize;
  const usize quatSlotOffset = slotOffset * 4;
  const int64 baseIndex = iz * m_BufSliceSize;

  AbstractDataStore<float32>& quatStore = m_QuatsArray->getDataStoreRef();
  for(usize i = 0; i < sliceSize; i++)
  {
    const usize srcBase = static_cast<usize>(baseIndex + static_cast<int64>(i)) * 4;
    const usize dstBase = quatSlotOffset + i * 4;
    m_QuatBuffer[dstBase + 0] = quatStore[srcBase + 0];
    m_QuatBuffer[dstBase + 1] = quatStore[srcBase + 1];
    m_QuatBuffer[dstBase + 2] = quatStore[srcBase + 2];
    m_QuatBuffer[dstBase + 3] = quatStore[srcBase + 3];
  }

  AbstractDataStore<int32>& phaseStore = m_CellPhases->getDataStoreRef();
  for(usize i = 0; i < sliceSize; i++)
  {
    m_PhaseBuffer[slotOffset + i] = phaseStore[static_cast<usize>(baseIndex) + i];
  }

  if(m_InputValues->UseMask && m_GoodVoxelsArray != nullptr)
  {
    for(usize i = 0; i < sliceSize; i++)
    {
      m_MaskBuffer[slotOffset + i] = m_GoodVoxelsArray->isTrue(static_cast<usize>(baseIndex) + i) ? 1 : 0;
    }
  }
  else
  {
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }

  m_BufferedSliceZ[slot] = iz;
}

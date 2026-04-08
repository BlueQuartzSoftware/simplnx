#include "CAxisSegmentFeatures.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <memory>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
CAxisSegmentFeatures::CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
CAxisSegmentFeatures::~CAxisSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
// Segments a hexagonal EBSD dataset into features (grains) based on c-axis
// alignment. Two neighboring voxels are grouped into the same feature when
// their crystallographic c-axes (the [0001] direction) are aligned within a
// user-specified angular tolerance. Unlike EBSDSegmentFeatures which uses full
// misorientation via LaueOps, this filter only considers the c-axis direction,
// which is useful for analyzing basal texture in hexagonal materials.
//
// Pre-validation:
//   Before segmentation, every cell's phase is checked against the crystal
//   structure table. All phases must be hexagonal (Hexagonal_High 6/mmm or
//   Hexagonal_Low 6/m); if any non-hexagonal phase is found, the filter
//   returns an error because c-axis alignment is only meaningful for HCP.
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
Result<> CAxisSegmentFeatures::operator()()
{
  this->m_NeighborScheme = m_InputValues->NeighborScheme;
  auto* imageGeometry = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range&)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      return MakeErrorResult(-8362, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  // Loop through all the "Phase" cell values and validate that any phase found is
  // a hexagonal phase. This guards against there being multiple phases defined in
  // and EBSD file but the non-hexagonal phases were actually never found
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Cache crystal structures locally to avoid per-element OOC access (principle 9)
  const usize numPhases = crystalStructures.getNumberOfTuples();
  std::vector<uint32> crystalStructuresCache(numPhases);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), numPhases));

  usize numCells = m_CellPhases->getNumberOfTuples();
  // Use a Z-slice-sized batch for optimal OOC I/O (one HDF5 hyperslab per slice)
  SizeVec3 scanDims = imageGeometry->getDimensions();
  const usize k_ScanBatchSize = static_cast<usize>(scanDims[0]) * static_cast<usize>(scanDims[1]);
  auto phasesBuf = std::make_unique<int32[]>(k_ScanBatchSize);
  auto& phasesStore = m_CellPhases->getDataStoreRef();
  for(usize offset = 0; offset < numCells; offset += k_ScanBatchSize)
  {
    const usize batchSize = std::min(k_ScanBatchSize, numCells - offset);
    phasesStore.copyIntoBuffer(offset, nonstd::span<int32>(phasesBuf.get(), batchSize));
    for(usize i = 0; i < batchSize; i++)
    {
      int32 currentPhaseIdx = phasesBuf[i];
      const auto crystalStructureType = crystalStructuresCache[static_cast<usize>(currentPhaseIdx)];
      if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        return MakeErrorResult(-8363,
                               fmt::format("Input data is using {} type crystal structures but segmenting features via c-axis mis orientation requires all phases to be either Hexagonal-Low 6/m "
                                           "or Hexagonal-High 6/mmm type crystal structures.",
                                           CrystalStructureEnumToString(crystalStructureType)));
      }
    }
  }

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0);
  auto* active = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  active->fill(1);

  // Dispatch between DFS (in-core) and CCL (OOC) algorithms
  if(IsOutOfCore(*m_FeatureIdsArray) || ForceOocAlgorithm())
  {
    SizeVec3 udims = imageGeometry->getDimensions();
    allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

    auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
    executeCCL(imageGeometry, featureIdsStore);

    deallocateSliceBuffers();
  }
  else
  {
    execute(imageGeometry);
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
    ClusterUtilities::RandomizeFeatureIds(m_FeatureIdsArray->getDataStoreRef(), this->m_FoundFeatures + 1);
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
int64 CAxisSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  DataArray<int32>::store_type& featureIds = m_FeatureIdsArray->getDataStoreRef();
  const usize totalPoints = featureIds.getNumberOfTuples();
  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();

  // start with the next voxel after the last seed
  auto randPoint = static_cast<usize>(nextSeed);
  int64 seed = -1;
  while(seed == -1 && randPoint < totalPoints)
  {
    if(featureIds[randPoint] == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if((!m_InputValues->UseMask || m_GoodVoxelsArray->isTrue(randPoint)) && cellPhases[randPoint] > 0)
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
    featureIds[static_cast<usize>(seed)] = gnum;
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
//   3. The neighbor must have a c-axis aligned with the reference voxel.
//
// C-axis misalignment calculation:
//   - Both voxels must share the same phase (no cross-phase grouping).
//   - Quaternion orientations (QuatF, 4 floats) are extracted for both voxels.
//   - Each quaternion is converted to a 3x3 orientation matrix, which is then
//     transposed and multiplied by the crystal c-axis unit vector [0,0,1] to
//     obtain the sample-frame c-axis direction for each voxel.
//   - Both c-axis vectors are normalized so the dot product directly gives
//     the cosine of the angle between them.
//   - The dot product is clamped to [-1, 1] to guard against floating-point
//     error, then acos() gives the misalignment angle w (in radians).
//   - Because the c-axis is bidirectional (parallel and antiparallel are
//     equivalent), the check accepts w <= tolerance OR (pi - w) <= tolerance.
//   - If accepted, the neighbor's featureId is set to `gnum` as a side effect.
// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  bool group = false;

  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Float32Array& currentQuat = *m_QuatsArray;
  Int32Array& featureIds = *m_FeatureIdsArray;
  Int32Array& cellPhases = *m_CellPhases;

  bool neighborPointIsGood = false;
  if(m_GoodVoxelsArray != nullptr)
  {
    neighborPointIsGood = m_GoodVoxelsArray->isTrue(neighborpoint);
  }

  if(featureIds[neighborpoint] == 0 && (!m_InputValues->UseMask || neighborPointIsGood))
  {
    if(cellPhases[referencepoint] == cellPhases[neighborpoint])
    {
      const ebsdlib::QuatF q1(currentQuat[referencepoint * 4], currentQuat[referencepoint * 4 + 1], currentQuat[referencepoint * 4 + 2], currentQuat[referencepoint * 4 + 3]);
      const ebsdlib::QuatF q2(currentQuat[neighborpoint * 4 + 0], currentQuat[neighborpoint * 4 + 1], currentQuat[neighborpoint * 4 + 2], currentQuat[neighborpoint * 4 + 3]);

      const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
      const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

      // Convert the quaternion matrices to transposed g matrices so when caxis is multiplied by it, it will give the sample direction that the caxis is along
      Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
      Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

      // normalize so that the dot product can be taken below without
      // dividing by the magnitudes (they would be 1)
      c1.normalize();
      c2.normalize();

      // Validate value of w falls between [-1, 1] to ensure that acos returns a valid value
      float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
      w = std::acos(w);
      if(w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance)
      {
        group = true;
        featureIds[neighborpoint] = gnum;
      }
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
//   When m_UseSliceBuffers is true (OOC mode), the method checks whether the
//   voxel's Z-slice is currently loaded in one of the two buffer slots. The
//   slot lookup checks both m_BufferedSliceZ[0] and m_BufferedSliceZ[1] to
//   find which slot (if any) holds the target slice. If found, mask and phase
//   values are read from the in-memory m_MaskBuffer and m_PhaseBuffer arrays,
//   avoiding an on-disk I/O round-trip.
//
// OOC fallback:
//   If slice buffers are not active, or if the voxel's slice is not currently
//   buffered (which can happen during Phase 1b of CCL when periodic boundary
//   merging accesses non-adjacent slices), the method falls back to direct
//   array access through the DataStore, which may trigger on-disk I/O for
//   out-of-core data.
// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    int64 sliceZ = point / m_BufSliceSize;
    if(sliceZ == m_BufferedSliceZ[0] || sliceZ == m_BufferedSliceZ[1])
    {
      int64 slot = (sliceZ == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 offset = point - sliceZ * m_BufSliceSize;
      int64 bufIdx = slot * m_BufSliceSize + offset;
      // Check mask
      if(m_InputValues->UseMask && m_MaskBuffer[bufIdx] == 0)
      {
        return false;
      }
      // Check phase
      if(m_PhaseBuffer[bufIdx] <= 0)
      {
        return false;
      }
      return true;
    }
  }

  // Fallback: direct array access
  if(m_InputValues->UseMask && !m_GoodVoxelsArray->isTrue(point))
  {
    return false;
  }
  Int32Array& cellPhases = *m_CellPhases;
  if(cellPhases[point] <= 0)
  {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Determines whether two neighboring voxels have sufficiently aligned c-axes
// to belong to the same feature. Used exclusively by the CCL path
// (executeCCL()), whereas the DFS path uses determineGrouping() instead.
//
// Slice buffer fast path:
//   When both voxels' Z-slices are present in the rolling 2-slot buffer, all
//   data is read from the in-memory buffers (m_QuatBuffer, m_PhaseBuffer,
//   m_MaskBuffer). The buffer index for each point is computed as:
//     slot * sliceSize + (point - sliceZ * sliceSize)
//   For quaternions, an additional x4 factor accounts for the 4 components
//   per voxel. The method then:
//     1. Checks point2's mask validity.
//     2. Checks that point2's phase > 0 and both phases match.
//     3. Constructs QuatF objects from the buffered quaternion components.
//     4. Converts each quaternion to an orientation matrix, transposes it, and
//        multiplies by [0,0,1] to get the sample-frame c-axis direction.
//     5. Normalizes both c-axis vectors and computes the dot product.
//     6. Clamps the dot product to [-1,1] and takes acos() to get the
//        misalignment angle w.
//     7. Returns true if w <= tolerance OR (pi - w) <= tolerance (because
//        parallel and antiparallel c-axes are crystallographically equivalent).
//
// OOC fallback:
//   If either voxel's slice is not buffered (e.g., during Phase 1b periodic
//   merge), falls back to direct DataStore access: validates point2 via
//   isValidVoxel(), checks phase equality, then computes c-axis misalignment
//   from the full quaternion and phase arrays on disk.
// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    int64 sliceZ1 = point1 / m_BufSliceSize;
    int64 sliceZ2 = point2 / m_BufSliceSize;
    bool buf1 = (sliceZ1 == m_BufferedSliceZ[0] || sliceZ1 == m_BufferedSliceZ[1]);
    bool buf2 = (sliceZ2 == m_BufferedSliceZ[0] || sliceZ2 == m_BufferedSliceZ[1]);

    if(buf1 && buf2)
    {
      int64 slot1 = (sliceZ1 == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 slot2 = (sliceZ2 == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 off1 = point1 - sliceZ1 * m_BufSliceSize;
      int64 off2 = point2 - sliceZ2 * m_BufSliceSize;
      int64 bufIdx1 = slot1 * m_BufSliceSize + off1;
      int64 bufIdx2 = slot2 * m_BufSliceSize + off2;

      // Check point2 validity (mask + phase)
      if(m_InputValues->UseMask && m_MaskBuffer[bufIdx2] == 0)
      {
        return false;
      }
      if(m_PhaseBuffer[bufIdx2] <= 0)
      {
        return false;
      }

      // Must be same phase
      if(m_PhaseBuffer[bufIdx1] != m_PhaseBuffer[bufIdx2])
      {
        return false;
      }

      // Read quaternions from buffer
      int64 qIdx1 = bufIdx1 * 4;
      int64 qIdx2 = bufIdx2 * 4;
      const ebsdlib::QuatF q1(m_QuatBuffer[qIdx1], m_QuatBuffer[qIdx1 + 1], m_QuatBuffer[qIdx1 + 2], m_QuatBuffer[qIdx1 + 3]);
      const ebsdlib::QuatF q2(m_QuatBuffer[qIdx2], m_QuatBuffer[qIdx2 + 1], m_QuatBuffer[qIdx2 + 2], m_QuatBuffer[qIdx2 + 3]);

      const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
      const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

      const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
      Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
      Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

      c1.normalize();
      c2.normalize();

      float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
      w = std::acos(w);

      return w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance;
    }
  }

  // Fallback: direct array access
  if(!isValidVoxel(point2))
  {
    return false;
  }

  Int32Array& cellPhases = *m_CellPhases;

  // Must be same phase
  if(cellPhases[point1] != cellPhases[point2])
  {
    return false;
  }

  // Calculate c-axis misalignment
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Float32Array& quats = *m_QuatsArray;

  const ebsdlib::QuatF q1(quats[point1 * 4], quats[point1 * 4 + 1], quats[point1 * 4 + 2], quats[point1 * 4 + 3]);
  const ebsdlib::QuatF q2(quats[point2 * 4], quats[point2 * 4 + 1], quats[point2 * 4 + 2], quats[point2 * 4 + 3]);

  const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
  const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

  Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
  Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

  c1.normalize();
  c2.normalize();

  float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
  w = std::acos(w);

  return w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance;
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
void CAxisSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  int64 totalSlots = 2 * m_BufSliceSize;
  m_QuatBuffer.resize(static_cast<usize>(totalSlots * 4));
  m_PhaseBuffer.resize(static_cast<usize>(totalSlots));
  m_MaskBuffer.resize(static_cast<usize>(totalSlots));
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_UseSliceBuffers = true;
}

// -----------------------------------------------------------------------------
// Releases the slice buffers after executeCCL() completes, freeing the memory
// back to the system. Called in the OOC branch of operator() after the CCL
// algorithm finishes. Resets m_UseSliceBuffers to false and both
// m_BufferedSliceZ slots to -1. Uses clear() + shrink_to_fit() on each vector
// to guarantee memory deallocation.
// -----------------------------------------------------------------------------
void CAxisSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_QuatBuffer.clear();
  m_QuatBuffer.shrink_to_fit();
  m_PhaseBuffer.clear();
  m_PhaseBuffer.shrink_to_fit();
  m_MaskBuffer.clear();
  m_MaskBuffer.shrink_to_fit();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufSliceSize = 0;
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
// Data loaded per slice:
//   - Quaternions (4 float32 per voxel) into m_QuatBuffer
//   - Phase IDs (1 int32 per voxel) into m_PhaseBuffer
//   - Mask flags (1 uint8 per voxel) into m_MaskBuffer; if masking is disabled,
//     all mask values are set to 1 (valid)
//
// Note: Unlike the EBSDSegmentFeatures version, this implementation does not
// include a skip-if-already-loaded check; the slot is always overwritten.
// -----------------------------------------------------------------------------
void CAxisSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return;
  }

  int64 slot = iz % 2;
  m_BufferedSliceZ[slot] = iz;

  const int64 sliceStart = iz * m_BufSliceSize;
  const int64 bufOffset = slot * m_BufSliceSize;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(bufOffset);

  // Bulk-read quaternions (4 components per voxel) for this slice
  AbstractDataStore<float32>& quatStore = m_QuatsArray->getDataStoreRef();
  quatStore.copyIntoBuffer(static_cast<usize>(sliceStart) * 4, nonstd::span<float32>(m_QuatBuffer.data() + slotOffset * 4, sliceSize * 4));

  // Bulk-read phase IDs for this slice
  AbstractDataStore<int32>& phaseStore = m_CellPhases->getDataStoreRef();
  phaseStore.copyIntoBuffer(static_cast<usize>(sliceStart), nonstd::span<int32>(m_PhaseBuffer.data() + slotOffset, sliceSize));

  // Bulk-read mask flags for this slice
  if(m_InputValues->UseMask && m_GoodVoxelsArray != nullptr)
  {
    auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(maskArray.getDataType() == DataType::uint8)
    {
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
      typedStore.copyIntoBuffer(static_cast<usize>(sliceStart), nonstd::span<uint8>(m_MaskBuffer.data() + slotOffset, sliceSize));
    }
    else if(maskArray.getDataType() == DataType::boolean)
    {
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<bool>>();
      auto boolBuf = std::make_unique<bool[]>(sliceSize);
      typedStore.copyIntoBuffer(static_cast<usize>(sliceStart), nonstd::span<bool>(boolBuf.get(), sliceSize));
      for(usize i = 0; i < sliceSize; i++)
      {
        m_MaskBuffer[slotOffset + i] = boolBuf[i] ? 1 : 0;
      }
    }
  }
  else
  {
    // If no mask, mark everything as valid
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }
}

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <vector>

namespace nx::core
{

/**
 * @struct EBSDSegmentFeaturesInputValues
 * @brief Holds all user-supplied parameters for the EBSDSegmentFeatures algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeaturesInputValues
{
  float32 MisorientationTolerance = 0.0f;           ///< Maximum misorientation angle (radians) for grouping voxels into the same feature
  bool UseMask = false;                             ///< Whether to exclude masked voxels from segmentation
  bool RandomizeFeatureIds = false;                 ///< Whether to randomize feature IDs after segmentation for better visual contrast
  SegmentFeatures::NeighborScheme NeighborScheme{}; ///< Face-only (6) or all-connected (26) neighbor connectivity
  DataPath ImageGeometryPath;                       ///< Path to the IGridGeometry defining the 3D voxel grid
  DataPath QuatsArrayPath;                          ///< Path to the Float32 quaternion array (4 components per cell)
  DataPath CellPhasesArrayPath;                     ///< Path to the Int32 cell phases array
  DataPath MaskArrayPath;                           ///< Path to the Bool/UInt8 mask array (only used when UseMask is true)
  DataPath CrystalStructuresArrayPath;              ///< Path to the UInt32 crystal structures ensemble array
  DataPath FeatureIdsArrayPath;                     ///< Path to the output Int32 feature IDs array
  DataPath CellFeatureAttributeMatrixPath;          ///< Path to the Feature-level AttributeMatrix (resized to match feature count)
  DataPath ActiveArrayPath;                         ///< Path to the output UInt8 Active array (1 = active feature, 0 = reserved slot 0)
  bool IsPeriodic = false;                          ///< Whether to apply periodic boundary conditions during segmentation
};

/**
 * @class EBSDSegmentFeatures
 * @brief Segments an EBSD dataset into crystallographic features (grains)
 * by flood-filling contiguous voxels whose orientations are within a
 * user-specified misorientation tolerance.
 *
 * Two neighboring voxels are grouped into the same feature only if they share
 * the same crystallographic phase and their misorientation -- computed via the
 * appropriate EbsdLib LaueOps symmetry operator from their quaternion
 * representations -- is below MisorientationTolerance. The misorientation
 * calculation accounts for all symmetry-equivalent orientations of the given
 * Laue class, so that the minimum rotation angle between the two crystal
 * orientations is used.
 *
 * ## Algorithm Dispatch
 *
 * The operator() dispatches between two segmentation strategies:
 *   - **In-core (DFS flood fill)**: The classic depth-first-search approach
 *     inherited from SegmentFeatures::execute(). Accesses DataArrays directly
 *     via operator[] for each voxel and neighbor.
 *   - **Out-of-core (CCL)**: A connected-component labeling algorithm via
 *     SegmentFeatures::executeCCL() that processes data slice-by-slice.
 *     Selected when IsOutOfCore() detects the FeatureIds array is backed by
 *     an on-disk DataStore, or when ForceOocAlgorithm() is set for testing.
 *
 * ## Rolling 2-Slot Slice Buffers (OOC Optimization)
 *
 * The CCL algorithm calls isValidVoxel() and areNeighborsSimilar() for every
 * voxel and its neighbors. On OOC DataStores, each call would trigger a chunk
 * decompress-read-evict cycle, making the algorithm orders of magnitude slower.
 *
 * To avoid this, prepareForSlice() bulk-reads each Z-slice's quaternion, phase,
 * and mask data into a rolling 2-slot buffer (even slices -> slot 0, odd slices
 * -> slot 1). Because CCL processes slices sequentially and only compares
 * adjacent slices, both the current slice (iz) and previous slice (iz-1) are
 * always resident. The isValidVoxel() and areNeighborsSimilar() methods check
 * the buffer first (fast path) and fall back to direct DataStore access only
 * when the needed slice is not buffered (e.g., during periodic boundary merging).
 *
 * Additionally, the small ensemble-level crystal structures array is cached
 * locally in m_CrystalStructuresCache to avoid per-voxel virtual dispatch
 * through the DataStore.
 */
class ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeatures : public SegmentFeatures
{
public:
  using FeatureIdsArrayType = Int32Array; ///< Type alias for the feature IDs array

  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Handler for sending progress messages to the UI.
   * @param shouldCancel Atomic flag checked between iterations to support cancellation.
   * @param inputValues User-supplied parameters controlling the segmentation behavior.
   */
  EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues);
  ~EBSDSegmentFeatures() noexcept override;

  EBSDSegmentFeatures(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures(EBSDSegmentFeatures&&) = delete;
  EBSDSegmentFeatures& operator=(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures& operator=(EBSDSegmentFeatures&&) = delete;

  /**
   * @brief Executes the EBSD segmentation algorithm, dispatching between DFS
   * (in-core) and CCL (OOC) strategies based on the backing DataStore type.
   * @return Result<> indicating success or any errors encountered during execution.
   */
  Result<> operator()();

protected:
  /**
   * @brief Finds the next unassigned voxel to serve as a seed for a new feature.
   * Used by the DFS flood-fill path (execute()).
   * @param gnum The feature number to assign to the seed voxel.
   * @param nextSeed Linear index to start scanning from.
   * @return Linear index of the seed voxel, or -1 if no more seeds exist.
   */
  int64 getSeed(int32 gnum, int64 nextSeed) const override;

  /**
   * @brief Determines whether a neighbor should be merged into the current
   * feature during DFS flood fill. Checks mask, phase equality, and
   * crystallographic misorientation via LaueOps.
   * @param referencePoint Linear index of the current (reference) voxel.
   * @param neighborPoint Linear index of the candidate neighbor voxel.
   * @param gnum The feature number to assign if the neighbor is accepted.
   * @return true if the neighbor was merged (its featureId set to gnum).
   */
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in EBSD segmentation.
   *
   * Used by the CCL path. When slice buffers are active, reads from the
   * in-memory buffer (fast path); otherwise falls back to direct DataStore
   * access (OOC fallback).
   *
   * @param point Linear voxel index.
   * @return true if the voxel passes mask and phase > 0 checks.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Determines whether two neighboring voxels belong to the same EBSD
   * segment, used by the CCL path.
   *
   * When both voxels' Z-slices are in the rolling buffer, all data is read
   * from local memory. Otherwise falls back to direct DataStore access.
   *
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both share the same phase and their misorientation is within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Pre-loads a Z-slice's quaternion, phase, and mask data into the
   * rolling 2-slot buffer for OOC-efficient access during CCL.
   *
   * Slot assignment: even slices -> slot 0, odd slices -> slot 1. Passing
   * iz < 0 disables slice buffering (used after the slice-by-slice sweep
   * completes, before periodic boundary merging).
   *
   * @param iz Z-slice index to load, or -1 to disable buffering.
   * @param dimX Number of voxels in X.
   * @param dimY Number of voxels in Y.
   * @param dimZ Number of voxels in Z.
   */
  void prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  const EBSDSegmentFeaturesInputValues* m_InputValues = nullptr;                  ///< Non-owning pointer to user-supplied parameters
  Float32Array* m_QuatsArray = nullptr;                                           ///< Pointer to the quaternion array (4 components per cell)
  FeatureIdsArrayType* m_CellPhases = nullptr;                                    ///< Pointer to the cell phases array
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr; ///< Mask comparator for filtering valid voxels
  DataArray<uint32>* m_CrystalStructures = nullptr;                               ///< Pointer to the crystal structures ensemble array

  FeatureIdsArrayType* m_FeatureIdsArray = nullptr; ///< Pointer to the output feature IDs array

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps; ///< Cached Laue symmetry operators for all crystal systems

  /**
   * @brief Allocates the rolling 2-slot slice buffers for OOC optimization.
   * Each slot holds one full XY slice of quaternions, phases, and mask flags.
   * @param dimX Number of voxels in X.
   * @param dimY Number of voxels in Y.
   */
  void allocateSliceBuffers(int64 dimX, int64 dimY);

  /**
   * @brief Releases the slice buffers and resets m_UseSliceBuffers to false.
   */
  void deallocateSliceBuffers();

  // Rolling 2-slot input buffers for OOC optimization.
  // Pre-loading input data into these avoids per-element OOC overhead
  // during neighbor comparisons in the CCL algorithm.
  std::vector<float32> m_QuatBuffer;            ///< 2 * sliceSize * 4 quaternion components
  std::vector<int32> m_PhaseBuffer;             ///< 2 * sliceSize phase IDs
  std::vector<uint8> m_MaskBuffer;              ///< 2 * sliceSize mask flags
  std::vector<uint32> m_CrystalStructuresCache; ///< Local copy of the crystal structures ensemble array
  int64 m_BufSliceSize = 0;                     ///< Number of voxels per XY slice (dimX * dimY)
  int64 m_BufferedSliceZ[2] = {-1, -1};         ///< Z-indices currently loaded in each buffer slot
  bool m_UseSliceBuffers = false;               ///< Whether slice buffers are active
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

namespace nx::core
{

/**
 * @struct CAxisSegmentFeaturesInputValues
 * @brief Holds all user-supplied parameters for the CAxisSegmentFeatures algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeaturesInputValues
{
  float32 MisorientationTolerance = 0.0f;           ///< Maximum c-axis misalignment angle (radians) for grouping voxels into the same feature
  bool UseMask = false;                             ///< Whether to exclude masked voxels from segmentation
  bool RandomizeFeatureIds = false;                 ///< Whether to randomize feature IDs after segmentation for better visual contrast
  SegmentFeatures::NeighborScheme NeighborScheme{}; ///< Face-only (6) or all-connected (26) neighbor connectivity
  DataPath ImageGeometryPath;                       ///< Path to the ImageGeom defining the 3D voxel grid
  DataPath QuatsArrayPath;                          ///< Path to the Float32 quaternion array (4 components per cell)
  DataPath CellPhasesArrayPath;                     ///< Path to the Int32 cell phases array
  DataPath MaskArrayPath;                           ///< Path to the Bool/UInt8 mask array (only used when UseMask is true)
  DataPath CrystalStructuresArrayPath;              ///< Path to the UInt32 crystal structures ensemble array
  DataPath FeatureIdsArrayPath;                     ///< Path to the output Int32 feature IDs array
  DataPath CellFeatureAttributeMatrixPath;          ///< Path to the Feature-level AttributeMatrix (resized to match feature count)
  DataPath ActiveArrayPath;                         ///< Path to the output UInt8 Active array (1 = active feature, 0 = reserved slot 0)
};

/**
 * @class CAxisSegmentFeatures
 * @brief Segments a hexagonal EBSD dataset into features (grains) based on
 * c-axis alignment rather than full crystallographic misorientation.
 *
 * The c-axis is the [0001] direction in hexagonal crystal systems (HCP metals
 * like titanium, zirconium, magnesium). Two neighboring voxels are grouped into
 * the same feature when the angle between their sample-frame c-axis directions
 * is within MisorientationTolerance. Because the c-axis is bidirectional
 * (parallel and antiparallel are equivalent), the check accepts both
 * w <= tolerance and (pi - w) <= tolerance.
 *
 * The c-axis direction is obtained by converting each voxel's quaternion
 * orientation to a 3x3 orientation matrix, transposing it, and multiplying by
 * the crystal-frame c-axis unit vector [0,0,1] to get the sample-frame direction.
 *
 * This filter requires all phases to be hexagonal (Hexagonal_High 6/mmm or
 * Hexagonal_Low 6/m). A pre-validation pass checks every cell's phase; if
 * any non-hexagonal phase is found, the filter returns an error.
 *
 * ## Algorithm Dispatch
 *
 * Identical to EBSDSegmentFeatures: dispatches between DFS flood fill (in-core)
 * and connected-component labeling (OOC) based on IsOutOfCore() or
 * ForceOocAlgorithm().
 *
 * ## Rolling 2-Slot Slice Buffers (OOC Optimization)
 *
 * Same architecture as EBSDSegmentFeatures: prepareForSlice() bulk-reads each
 * Z-slice's quaternion, phase, and mask data into a rolling 2-slot buffer.
 * The isValidVoxel() and areNeighborsSimilar() methods use the buffer fast path
 * when the needed slices are loaded, falling back to direct DataStore access
 * for non-adjacent slice comparisons during periodic boundary merging.
 *
 * The pre-validation phase scan also uses OOC-efficient batch reading: phases
 * are read one Z-slice at a time via copyIntoBuffer() rather than per-element
 * getValue() calls.
 */
class ORIENTATIONANALYSIS_EXPORT CAxisSegmentFeatures : public SegmentFeatures
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Handler for sending progress messages to the UI.
   * @param shouldCancel Atomic flag checked between iterations to support cancellation.
   * @param inputValues User-supplied parameters controlling the segmentation behavior.
   */
  CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues);
  ~CAxisSegmentFeatures() noexcept override;

  CAxisSegmentFeatures(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures(CAxisSegmentFeatures&&) noexcept = delete;
  CAxisSegmentFeatures& operator=(const CAxisSegmentFeatures&) = delete;
  CAxisSegmentFeatures& operator=(CAxisSegmentFeatures&&) noexcept = delete;

  /**
   * @brief Executes the c-axis segmentation algorithm, dispatching between DFS
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
   * c-axis alignment (not full misorientation).
   * @param referencePoint Linear index of the current (reference) voxel.
   * @param neighborPoint Linear index of the candidate neighbor voxel.
   * @param gnum The feature number to assign if the neighbor is accepted.
   * @return true if the neighbor was merged (its featureId set to gnum).
   */
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in c-axis segmentation.
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
   * @brief Determines whether two neighboring voxels have sufficiently aligned
   * c-axes to belong to the same feature. Used by the CCL path.
   *
   * When both voxels' Z-slices are in the rolling buffer, all data is read
   * from local memory. Otherwise falls back to direct DataStore access.
   *
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both share the same phase and their c-axis misalignment is within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Pre-loads a Z-slice's quaternion, phase, and mask data into the
   * rolling 2-slot buffer for OOC-efficient access during CCL.
   *
   * Slot assignment: even slices -> slot 0, odd slices -> slot 1. Passing
   * iz < 0 disables slice buffering.
   *
   * @param iz Z-slice index to load, or -1 to disable buffering.
   * @param dimX Number of voxels in X.
   * @param dimY Number of voxels in Y.
   * @param dimZ Number of voxels in Z.
   */
  void prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  const CAxisSegmentFeaturesInputValues* m_InputValues = nullptr; ///< Non-owning pointer to user-supplied parameters

  Float32Array* m_QuatsArray = nullptr;                                           ///< Pointer to the quaternion array (4 components per cell)
  Int32Array* m_CellPhases = nullptr;                                             ///< Pointer to the cell phases array
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr; ///< Mask comparator for filtering valid voxels
  Int32Array* m_FeatureIdsArray = nullptr;                                        ///< Pointer to the output feature IDs array

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
  std::vector<float32> m_QuatBuffer;    ///< 2 * sliceSize * 4 quaternion components
  std::vector<int32> m_PhaseBuffer;     ///< 2 * sliceSize phase IDs
  std::vector<uint8> m_MaskBuffer;      ///< 2 * sliceSize mask flags
  int64 m_BufSliceSize = 0;             ///< Number of voxels per XY slice (dimX * dimY)
  int64 m_BufferedSliceZ[2] = {-1, -1}; ///< Z-indices currently loaded in each buffer slot
  bool m_UseSliceBuffers = false;       ///< Whether slice buffers are active
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <random>
#include <vector>

namespace nx::core
{

/**
 * @struct ScalarSegmentFeaturesInputValues
 * @brief Holds all user-configured parameters for the ScalarSegmentFeatures algorithm.
 */
struct SIMPLNXCORE_EXPORT ScalarSegmentFeaturesInputValues
{
  int ScalarTolerance = 0;                        ///< Maximum absolute difference between neighboring voxels for grouping.
  bool UseMask;                                   ///< If true, only voxels flagged as "good" in the mask participate.
  bool RandomizeFeatureIds;                       ///< If true, randomize Feature IDs post-segmentation for visual contrast.
  bool IsPeriodic = false;                        ///< If true, treat geometry boundaries as periodic (tileable).
  SegmentFeatures::NeighborScheme NeighborScheme; ///< 6-face or 26-connected neighbor scheme.
  DataPath ImageGeometryPath;                     ///< Path to the ImageGeom / IGridGeometry being segmented.
  DataPath InputDataPath;                         ///< Path to the scalar array used for comparison (any numeric type).
  DataPath MaskArrayPath;                         ///< Path to the boolean/uint8 mask array (used when UseMask is true).
  DataPath FeatureIdsArrayPath;                   ///< Output: per-cell Feature ID array (int32).
  DataPath CellFeatureAttributeMatrixPath;        ///< Output: Attribute Matrix for per-feature arrays.
  DataPath ActiveArrayPath;                       ///< Output: boolean array marking active features.
};

/**
 * @class ScalarSegmentFeatures
 * @brief Segments an ImageGeom into features by flood-filling contiguous voxels
 * whose scalar values differ by no more than a user-specified tolerance.
 *
 * This is a general-purpose segmentation algorithm that works on any single-component
 * scalar array (int8 through float64, plus boolean), unlike orientation-based
 * segmentation filters (EBSD, CAxis). The tolerance defines the maximum absolute
 * difference between neighboring voxels for them to be grouped into the same feature.
 *
 * @section dual_path Dual Algorithm Paths
 * The algorithm has two execution paths selected automatically at runtime:
 * - **In-core (DFS flood fill)**: Classic depth-first search via the base-class
 *   execute() method. Uses per-element access through the typed CompareFunctor.
 * - **Out-of-core (CCL)**: Connected-component labeling via executeCCL(), which
 *   processes data Z-slice-by-Z-slice to limit memory usage. Activated when the
 *   FeatureIds array is backed by an OOC DataStore or ForceOocAlgorithm() is set.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The CCL path uses a rolling 2-slot buffer system to avoid per-element virtual
 * dispatch overhead on OOC DataStores. Before processing each Z-slice, prepareForSlice()
 * bulk-reads the scalar input and mask arrays for that slice into contiguous
 * in-memory buffers via copyIntoBuffer(). The isValidVoxel() and areNeighborsSimilar()
 * overrides then read from these buffers instead of the underlying DataStore,
 * eliminating chunk load/evict cycles. Two slots are needed because CCL compares
 * the current slice with the previous slice (iz and iz-1).
 *
 * All scalar types are converted to float64 in the buffer for uniform comparison,
 * and the tolerance is also cast to float64.
 */
class SIMPLNXCORE_EXPORT ScalarSegmentFeatures : public SegmentFeatures
{
public:
  using FeatureIdsArrayType = Int32Array;
  using GoodVoxelsArrayType = BoolArray;

  ScalarSegmentFeatures(DataStructure& dataStructure, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ScalarSegmentFeatures() noexcept override;

  ScalarSegmentFeatures(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures(ScalarSegmentFeatures&&) noexcept = delete;
  ScalarSegmentFeatures& operator=(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures& operator=(ScalarSegmentFeatures&&) noexcept = delete;

  /**
   * @brief Executes the segmentation: sets up comparators, dispatches to DFS or CCL,
   * then post-processes (resize AM, fill Active array, optionally randomize IDs).
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

protected:
  /**
   * @brief Finds the next unassigned voxel to seed a new feature (DFS path).
   * @param gnum The feature number to assign to the seed.
   * @param nextSeed Linear index to start scanning from.
   * @return Linear index of the seed voxel, or -1 if no more seeds exist.
   */
  int64 getSeed(int32 gnum, int64 nextSeed) const override;

  /**
   * @brief Determines whether a neighbor should be merged into the current feature (DFS path).
   * Checks featureId == 0, mask validity, then delegates to the typed CompareFunctor.
   * @param referencePoint Linear index of the reference voxel.
   * @param neighborPoint Linear index of the candidate neighbor.
   * @param gnum Current feature number (assigned to neighbor on success).
   * @return true if the neighbor was merged into the feature.
   */
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

  /**
   * @brief Checks whether a voxel can participate in segmentation (CCL path).
   * Uses the slice buffer fast path when available; falls back to direct MaskCompare access.
   * @param point Linear voxel index.
   * @return true if the voxel passes the mask check (or no mask is used).
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Determines whether two neighboring voxels have similar enough scalar values
   * to belong to the same feature (CCL path). Uses the slice buffer fast path when
   * both voxels' Z-slices are buffered; falls back to CompareFunctor otherwise.
   * @param point1 First voxel index.
   * @param point2 Second (neighbor) voxel index.
   * @return true if both voxels are valid and their scalar values are within tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Pre-loads input scalar and mask data for the given Z-slice into the
   * rolling 2-slot buffer, eliminating per-element OOC overhead during CCL.
   *
   * Slot assignment: even slices go to slot 0, odd to slot 1. This ensures that
   * both the current slice and the previous slice are always in memory.
   * Passing iz = -1 disables buffering (used after the slice sweep for Phase 1b).
   *
   * @param iz Current Z-slice index, or -1 to disable buffering.
   * @param dimX X dimension of the grid.
   * @param dimY Y dimension of the grid.
   * @param dimZ Z dimension of the grid.
   */
  void prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  /**
   * @brief Allocates the rolling 2-slot buffers for scalar and mask data.
   * Each slot holds dimX * dimY elements (one full XY slice).
   * @param dimX X dimension of the grid.
   * @param dimY Y dimension of the grid.
   */
  void allocateSliceBuffers(int64 dimX, int64 dimY);

  /**
   * @brief Releases the slice buffers and resets buffering state.
   */
  void deallocateSliceBuffers();

  const ScalarSegmentFeaturesInputValues* m_InputValues = nullptr;           ///< User-configured parameters.
  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;                          ///< Output Feature IDs array.
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;                          ///< Good voxels mask (if used).
  std::shared_ptr<SegmentFeatures::CompareFunctor> m_CompareFunctor;         ///< Typed comparator for DFS path.
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxels = nullptr; ///< Mask comparator.
  IDataArray* m_InputDataArray = nullptr;                                    ///< Raw pointer to the input scalar array (for type dispatch).

  // --- Rolling 2-slot input buffers for OOC optimization ---
  std::vector<float64> m_ScalarBuffer;  ///< Scalar values as float64 for uniform comparison (2 * sliceSize elements).
  std::vector<uint8> m_MaskBuffer;      ///< Mask flags as uint8 (2 * sliceSize elements; 0 = masked out, 1 = valid).
  int64 m_BufSliceSize = 0;             ///< Number of voxels per XY slice (dimX * dimY).
  int64 m_BufferedSliceZ[2] = {-1, -1}; ///< Z-index currently loaded in each slot (-1 = empty).
  bool m_UseSliceBuffers = false;       ///< True when the CCL path has activated slice buffering.
};
} // namespace nx::core

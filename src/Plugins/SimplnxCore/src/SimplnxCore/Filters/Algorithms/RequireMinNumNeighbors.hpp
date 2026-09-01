#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct RequireMinNumNeighborsInputValues
 * @brief Stores thresholds, phase selection, paths, and ignored cell arrays.
 */
struct SIMPLNXCORE_EXPORT RequireMinNumNeighborsInputValues
{
  bool ApplyToSinglePhase;
  DataPath FeaturePhasesPath;
  uint64 PhaseNumber;
  uint64 MinNumNeighbors;
  DataPath ImageGeomPath;
  DataPath FeatureIdsPath;
  DataPath NumNeighborsPath;
  MultiArraySelectionParameter::ValueType IgnoredVoxelArrayPaths;
};

/**
 * @class RequireMinNumNeighbors
 * @brief Removes features below a neighbor-count threshold and fills their cells.
 *
 * Single-phase mode removes only features in the selected phase. Other phases
 * remain active. Surviving Feature IDs are compacted during the same 65,536-cell
 * pass that marks removed cells as negative. This avoids a second complete
 * Feature ID renumbering pass.
 *
 * FillBadVoxels processes cell arrays sequentially through rolling slices. It
 * skips the selected ignored arrays and updates Feature IDs last. This order lets
 * each array use the same assignment snapshot. Cancellation between arrays can
 * leave a sibling array ahead of Feature IDs, so canceled output must be discarded.
 *
 * Resident scratch is feature-scale active and renumber state, one fixed Feature
 * ID chunk, and rolling slices for one cell array. The marking pass discards its
 * bulk-I/O results. A storage failure can return success with partial Feature IDs.
 */
class SIMPLNXCORE_EXPORT RequireMinNumNeighbors
{
public:
  /**
   * @brief Initializes the neighbor-count removal algorithm.
   * @param dataStructure Contains image, cell, and feature data.
   * @param mesgHandler Receives removal and fill messages.
   * @param shouldCancel Signals cancellation between phases, chunks, and slices.
   * @param inputValues Selects thresholds, paths, and ignored arrays.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  RequireMinNumNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinNumNeighborsInputValues* inputValues);
  /**
   * @brief Destroys the neighbor-count removal algorithm.
   */
  ~RequireMinNumNeighbors() noexcept;

  RequireMinNumNeighbors(const RequireMinNumNeighbors&) = delete;
  RequireMinNumNeighbors(RequireMinNumNeighbors&&) noexcept = delete;
  RequireMinNumNeighbors& operator=(const RequireMinNumNeighbors&) = delete;
  RequireMinNumNeighbors& operator=(RequireMinNumNeighbors&&) noexcept = delete;

  /**
   * @brief Removes selected features, fills their cells, and compacts feature data.
   * @return Phase, Feature ID, fill, or feature-compaction result.
   * @pre Cell arrays match the ImageGeom cell dimensions.
   *
   * Cancellation returns success without rollback. An error after marking starts
   * can leave compacted or negative Feature IDs and unmodified feature arrays.
   */
  Result<> operator()();

private:
  /**
   * @brief Selects active features and applies their compacted IDs to cells.
   * @param featureIds Feature IDs modified in place.
   * @param numNeighbors Supplies one neighbor count per feature.
   * @param totalPoints Number of ImageGeom cells to process.
   * @param errorReturn Receives an all-removed or range error.
   * @return Active flags indexed by original Feature ID. Returns empty after cancellation.
   *
   * A range error can occur after prior chunks changed. Bulk-I/O results are not
   * inspected. The shared renumber mapping must match feature-array compaction.
   */
  std::vector<bool> removeFeaturesUnderNeighborThreshold(Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& numNeighbors, usize totalPoints, Error& errorReturn);

  /**
   * @brief Fills negative Feature IDs from face-neighbor majority votes.
   * @param dimensions ImageGeom dimensions in X, Y, and Z order.
   * @param totalFeatures Exclusive upper bound for nonnegative Feature IDs.
   * @return Feature ID, unfillable-region, or bulk-transfer result.
   *
   * Nonignored sibling arrays update sequentially. Feature IDs update last.
   */
  Result<> assignBadVoxels(SizeVec3 dimensions, usize totalFeatures);

  DataStructure& m_DataStructure;
  const RequireMinNumNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

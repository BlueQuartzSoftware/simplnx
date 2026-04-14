#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/AlignSections.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct AlignSectionsMisorientationInputValues
 * @brief Holds all user-supplied parameters for the AlignSectionsMisorientation algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientationInputValues
{
  DataPath ImageGeometryPath; ///< Path to the IGridGeometry defining the 3D voxel grid
  bool UseMask = false;       ///< Whether to exclude masked voxels from the alignment cost function
  DataPath MaskArrayPath;     ///< Path to the Bool/UInt8 mask array (only used when UseMask is true)

  float32 misorientationTolerance = 0.0f; ///< Maximum misorientation angle (degrees) below which a cell pair is "aligned"
  DataPath quatsArrayPath;                ///< Path to the Float32 quaternion array (4 components per cell)
  DataPath cellPhasesArrayPath;           ///< Path to the Int32 cell phases array
  DataPath crystalStructuresArrayPath;    ///< Path to the UInt32 crystal structures ensemble array

  bool StoreAlignmentShifts = false;  ///< Whether to write computed shifts into output DataArrays
  DataPath AlignmentAMPath;           ///< Path to the Attribute Matrix storing alignment shift output arrays
  DataPath SlicesArrayPath;           ///< Path to the UInt32 array recording which slices were compared
  DataPath RelativeShiftsArrayPath;   ///< Path to the Int64 array recording per-iteration relative X/Y shifts
  DataPath CumulativeShiftsArrayPath; ///< Path to the Int64 array recording cumulative X/Y shifts
};

/**
 * @class AlignSectionsMisorientation
 * @brief Aligns consecutive Z-sections of an EBSD dataset by finding the X-Y
 * shift that minimizes cross-section crystallographic misorientation.
 *
 * For each pair of adjacent Z-slices the algorithm evaluates a 7x7 grid of
 * candidate X-Y shifts. At each candidate position, voxel pairs between the
 * two slices are sampled (every 4th voxel in X and Y) and the fraction of
 * pairs whose misorientation exceeds the user-specified tolerance is computed.
 * The candidate with the lowest fraction becomes the new grid center, and the
 * search repeats until convergence (a downhill-simplex-like iterative refinement).
 *
 * Misorientation is computed via EbsdLib LaueOps symmetry operators using the
 * quaternion representation stored in the input data. Only voxels that share
 * the same crystallographic phase are compared; cross-phase pairs are counted
 * as misoriented.
 *
 * ## Out-of-Core (OOC) Optimization
 *
 * When the input quaternion or phase arrays are backed by an out-of-core
 * (chunked, on-disk) DataStore, the in-core path would trigger thousands of
 * random chunk decompressions per slice pair because the 7x7 convergence loop
 * re-reads the same voxels many times. The OOC path (findShiftsOoc) instead:
 *   1. Pre-loads the topmost Z-slice into a "reference" buffer via bulk
 *      copyIntoBuffer() -- one contiguous read per array.
 *   2. For each subsequent slice, bulk-reads the "current" slice into a second
 *      buffer, then runs the identical convergence logic on the local buffers.
 *   3. After convergence, swaps the current buffer into the reference slot
 *      (O(1) std::swap), so the next iteration reuses the data without re-reading.
 *
 * This reduces per-slice I/O from O(thousands of random chunk reads) to exactly
 * 2 sequential bulk reads (phases + quats) plus an optional mask read.
 */
class ORIENTATIONANALYSIS_EXPORT AlignSectionsMisorientation : public AlignSections
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Handler for sending progress messages to the UI.
   * @param shouldCancel Atomic flag checked between iterations to support cancellation.
   * @param inputValues User-supplied parameters controlling the alignment behavior.
   */
  AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsMisorientationInputValues* inputValues);
  ~AlignSectionsMisorientation() noexcept override;

  AlignSectionsMisorientation(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation(AlignSectionsMisorientation&&) noexcept = delete;
  AlignSectionsMisorientation& operator=(const AlignSectionsMisorientation&) = delete;
  AlignSectionsMisorientation& operator=(AlignSectionsMisorientation&&) noexcept = delete;

  /**
   * @brief Executes the section alignment algorithm.
   * @return Result<> indicating success or any errors encountered during execution.
   */
  Result<> operator()();

protected:
  /**
   * @brief Computes the optimal X-Y shift for each pair of adjacent Z-slices.
   *
   * Dispatches to findShiftsOoc() when the input arrays are backed by an
   * out-of-core DataStore; otherwise runs the in-core path that accesses the
   * DataArrays directly via operator[].
   *
   * @param xShifts Output vector of cumulative X shifts per slice (size = numSlices).
   * @param yShifts Output vector of cumulative Y shifts per slice (size = numSlices).
   * @return Success or error result.
   */
  Result<> findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts) override;

private:
  /**
   * @brief OOC-optimized variant of findShifts that buffers two adjacent Z-slices
   * (quaternions, phases, and mask) into local std::vectors before the convergence
   * loop, eliminating per-tuple chunk thrashing on out-of-core DataStores.
   *
   * Uses a double-buffering strategy: after convergence for a slice pair, the
   * "current" buffer is swapped into the "reference" slot via std::swap, avoiding
   * a redundant re-read for the next iteration.
   *
   * @param xShifts Output vector of cumulative X shifts per slice.
   * @param yShifts Output vector of cumulative Y shifts per slice.
   * @return Success or error result.
   */
  Result<> findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts);

  DataStructure& m_DataStructure;                                        ///< Reference to the DataStructure containing all arrays
  const AlignSectionsMisorientationInputValues* m_InputValues = nullptr; ///< Non-owning pointer to the user-supplied parameters
  const std::atomic_bool& m_ShouldCancel;                                ///< Atomic flag for cooperative cancellation
  const IFilter::MessageHandler& m_MessageHandler;                       ///< Handler for sending progress messages to the UI
};
} // namespace nx::core

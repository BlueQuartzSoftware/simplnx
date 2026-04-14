#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeIPFColorsInputValues;

/**
 * @class ComputeIPFColorsDirect
 * @brief In-core (Direct) algorithm for computing Inverse Pole Figure colors.
 *
 * This algorithm is selected by the dispatcher when all relevant arrays reside
 * in contiguous in-memory DataStores. It uses ParallelDataAlgorithm to split
 * the voxel range across threads, with each thread computing IPF colors for its
 * assigned range by:
 *
 *   1. Reading Euler angles (phi1, Phi, phi2) for the voxel.
 *   2. Checking the phase ID against the crystal-structure ensemble array.
 *   3. Calling LaueOps::generateIPFColor() to transform the user-specified
 *      reference direction into the crystal frame and map it to an RGB color
 *      on the Laue-class-specific inverse pole figure triangle.
 *
 * An optional mask array allows pre-indexed voxels to be skipped (colored black).
 *
 * **Thread safety**: The parallel worker (ComputeIPFColorsImpl) accesses
 * AbstractDataStore references, which requires ParallelDataAlgorithm's
 * requireArraysInMemory() to lock the arrays in RAM for the duration of
 * execution. This is safe for in-core DataStores but would be dangerous for
 * OOC-backed stores.
 *
 * @see ComputeIPFColorsScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsDirect
{
public:
  /**
   * @brief Constructs the in-core IPF color algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param msgHandler Message handler for progress/warning messages.
   * @param shouldCancel Atomic cancellation flag checked inside the parallel worker.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  ComputeIPFColorsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColorsDirect() noexcept;

  ComputeIPFColorsDirect(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect(ComputeIPFColorsDirect&&) = delete;
  ComputeIPFColorsDirect& operator=(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect& operator=(ComputeIPFColorsDirect&&) = delete;

  /**
   * @brief Computes IPF colors for all voxels using multi-threaded random access.
   * @return Result<> with an error if phase data is inconsistent.
   */
  Result<> operator()();

  /**
   * @brief Thread-safe increment of the phase-mismatch warning counter.
   *
   * Called from parallel worker threads when a voxel's phase ID exceeds the
   * number of entries in the crystal structures ensemble array. The count is
   * not atomic (minor race is acceptable since it is only used for a warning
   * message), but incrementing is trivial and the final check is post-join.
   */
  void incrementPhaseWarningCount();

  /**
   * @brief Returns the current cancellation state.
   * @return true if the user has requested cancellation.
   */
  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;                             ///< Reference to the live DataStructure.
  const IFilter::MessageHandler& m_MessageHandler;            ///< Message handler for user-facing messages.
  const std::atomic_bool& m_ShouldCancel;                     ///< Cancellation flag.
  const ComputeIPFColorsInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
  int32_t m_PhaseWarningCount = 0;                            ///< Accumulates the number of voxels with out-of-range phase IDs.
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeIPFColorsInputValues;

/**
 * @class ComputeIPFColorsScanline
 * @brief Out-of-core (Scanline) algorithm for computing Inverse Pole Figure colors.
 *
 * This algorithm is selected by the dispatcher when any of the Euler-angle, phase,
 * or IPF-color arrays are backed by chunked (OOC) storage on disk. It avoids the
 * random-access pattern of the in-core path, which would trigger expensive
 * chunk load/evict cycles ("chunk thrashing") when data does not fit in RAM.
 *
 * **Strategy**: The algorithm processes voxels in sequential, fixed-size chunks
 * of k_ChunkTuples (65,536) tuples at a time:
 *
 *   1. Bulk-read Euler angles, phase IDs, and (optionally) the mask for the
 *      current chunk into local heap buffers via copyIntoBuffer().
 *   2. Compute IPF colors for every voxel in the chunk using the same
 *      LaueOps::generateIPFColor() logic as the in-core path.
 *   3. Bulk-write the computed RGB colors back via copyFromBuffer().
 *
 * Because OOC stores are chunked contiguously along the tuple dimension, this
 * linear access pattern reads each disk chunk at most once, achieving throughput
 * limited only by sequential disk I/O rather than random-access latency.
 *
 * **Single-threaded**: The algorithm runs single-threaded because the chunk
 * buffers are shared state and because OOC disk I/O does not benefit from
 * multi-threaded access to the same store.
 *
 * **Memory footprint**: O(k_ChunkTuples) per array -- roughly 1 MB total for
 * the default chunk size, regardless of dataset size.
 *
 * @see ComputeIPFColorsDirect for the multi-threaded in-core variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsScanline
{
public:
  /**
   * @brief Constructs the OOC IPF color algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param msgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic cancellation flag checked once per chunk.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  ComputeIPFColorsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColorsScanline() noexcept;

  ComputeIPFColorsScanline(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline(ComputeIPFColorsScanline&&) = delete;
  ComputeIPFColorsScanline& operator=(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline& operator=(ComputeIPFColorsScanline&&) = delete;

  /**
   * @brief Computes IPF colors for all voxels using sequential chunk-based I/O.
   * @return Result<> with an error if phase data is inconsistent.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                             ///< Reference to the live DataStructure.
  const IFilter::MessageHandler& m_MessageHandler;            ///< Message handler for user-facing messages.
  const std::atomic_bool& m_ShouldCancel;                     ///< Cancellation flag.
  const ComputeIPFColorsInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsScanline
 * @brief Out-of-core (OOC) optimized algorithm for counting boundary cells using a
 * Z-slice rolling window with sequential bulk I/O.
 *
 * **The problem this solves**: When the FeatureIds array is stored out-of-core in
 * chunked format (e.g., Zarr/HDF5 chunks on disk), each call to operator[] may
 * trigger a disk read to load the chunk containing that element. The boundary-cell
 * algorithm needs to access each voxel AND its 6 face neighbors. The +/-Z neighbors
 * are dimX*dimY elements away in the flat index space, which almost certainly live
 * in a different chunk. This means up to 3 chunk loads per voxel (current, prev-Z,
 * next-Z chunks), causing catastrophic "chunk thrashing" that makes the algorithm
 * 100-1000x slower than in-core execution.
 *
 * **How the rolling window solves it**: Instead of random operator[] access, this
 * algorithm reads one complete Z-slice at a time using copyIntoBuffer(), which
 * performs a single sequential bulk read per slice. Three std::vector<int32> buffers
 * hold the previous, current, and next Z-slices simultaneously:
 *
 *   - prevSlice: Z-slice at (z-1) -- needed for -Z neighbor lookups
 *   - curSlice:  Z-slice at (z)   -- the slice being processed
 *   - nextSlice: Z-slice at (z+1) -- needed for +Z neighbor lookups
 *
 * Within a single Z-slice, all X and Y neighbor lookups are simple index arithmetic
 * on the curSlice buffer (+/-1 for X, +/-dimX for Y). After processing curSlice,
 * the window shifts: prevSlice <- curSlice, curSlice <- nextSlice, and a new
 * nextSlice is loaded from disk. The output is similarly written one Z-slice at a
 * time using copyFromBuffer().
 *
 * This guarantees that the entire algorithm reads and writes the FeatureIds and
 * BoundaryCells arrays in strictly sequential order, with exactly one bulk I/O
 * operation per Z-slice -- optimal for chunked storage.
 *
 * **Memory overhead**: 3 input buffers (prevSlice, curSlice, nextSlice) each of
 * size dimX * dimY * sizeof(int32), plus 1 output buffer of size dimX * dimY *
 * sizeof(int8). For a 1000x1000 slice, this is approximately 12 MB total.
 *
 * @see ComputeBoundaryCellsDirect for the in-core variant.
 * @see ComputeBoundaryCells for the dispatcher.
 * @see DispatchAlgorithm for the selection mechanism.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsScanline
{
public:
  /**
   * @brief Constructs the OOC-optimized boundary cell counter.
   * @param dataStructure The DataStructure containing FeatureIds and BoundaryCells arrays.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, flags).
   */
  ComputeBoundaryCellsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsScanline() noexcept;

  ComputeBoundaryCellsScanline(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline(ComputeBoundaryCellsScanline&&) noexcept = delete;
  ComputeBoundaryCellsScanline& operator=(const ComputeBoundaryCellsScanline&) = delete;
  ComputeBoundaryCellsScanline& operator=(ComputeBoundaryCellsScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized boundary cell counting algorithm using
   * a 3-slice rolling window with copyIntoBuffer/copyFromBuffer bulk I/O.
   * @return Result<> indicating success or errors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                 ///< Reference to the DataStructure containing all data.
  const ComputeBoundaryCellsInputValues* m_InputValues = nullptr; ///< Algorithm parameters.
  const std::atomic_bool& m_ShouldCancel;                         ///< Cooperative cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                ///< Progress message handler.
};

} // namespace nx::core

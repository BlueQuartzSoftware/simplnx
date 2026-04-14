#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeBoundaryCellsInputValues;

/**
 * @class ComputeBoundaryCellsDirect
 * @brief In-core (direct memory access) algorithm for counting boundary cells.
 *
 * This is the traditional algorithm that uses operator[] to read FeatureIds and write
 * BoundaryCells directly through the DataStore abstraction. It iterates all voxels in
 * Z-Y-X order and, for each voxel, checks its 6 face-connected neighbors using
 * pre-computed index offsets from NeighborUtilities.hpp.
 *
 * **When this variant is selected**: DispatchAlgorithm selects this class when all
 * input arrays are backed by contiguous in-memory DataStore (i.e., not chunked/OOC).
 * With in-memory data, operator[] is a simple pointer dereference, so random neighbor
 * lookups are essentially free.
 *
 * **Why a separate OOC variant exists**: When FeatureIds is stored out-of-core in
 * chunked format (e.g., Zarr/HDF5 chunks), every operator[] call may trigger a chunk
 * load from disk. The 6-neighbor access pattern means up to 7 chunk loads per voxel
 * (the voxel itself plus its neighbors), which causes catastrophic "chunk thrashing"
 * and can slow the algorithm by 100-1000x. The Scanline variant avoids this by
 * reading entire Z-slices sequentially with copyIntoBuffer().
 *
 * @see ComputeBoundaryCellsScanline for the OOC-optimized variant.
 * @see ComputeBoundaryCells for the dispatcher.
 */
class SIMPLNXCORE_EXPORT ComputeBoundaryCellsDirect
{
public:
  /**
   * @brief Constructs the in-core boundary cell counter.
   * @param dataStructure The DataStructure containing FeatureIds and BoundaryCells arrays.
   * @param mesgHandler Handler for progress/info messages.
   * @param shouldCancel Atomic flag for cooperative cancellation.
   * @param inputValues Algorithm parameters (geometry path, array paths, flags).
   */
  ComputeBoundaryCellsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeBoundaryCellsInputValues* inputValues);
  ~ComputeBoundaryCellsDirect() noexcept;

  ComputeBoundaryCellsDirect(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect(ComputeBoundaryCellsDirect&&) noexcept = delete;
  ComputeBoundaryCellsDirect& operator=(const ComputeBoundaryCellsDirect&) = delete;
  ComputeBoundaryCellsDirect& operator=(ComputeBoundaryCellsDirect&&) noexcept = delete;

  /**
   * @brief Executes the in-core boundary cell counting algorithm.
   *
   * Iterates every voxel in Z-Y-X order and counts how many of its 6 face
   * neighbors belong to a different feature. Optionally counts volume-boundary
   * faces and optionally ignores feature 0 neighbors.
   *
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

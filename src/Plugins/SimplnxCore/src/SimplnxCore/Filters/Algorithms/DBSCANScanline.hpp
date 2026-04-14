#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANScanline
 * @brief Out-of-core algorithm for grid-based DBSCAN using chunked bulk I/O.
 *
 * The DBSCAN algorithm has two major data access phases that benefit from OOC optimization:
 *
 * **Grid construction** (bounds detection, binning, cell filling): The input array is
 * scanned multiple times to compute min/max bounds, bin each point into a grid cell,
 * and build the grid-to-point index. The Scanline variant reads the input array in
 * sequential 64K-tuple chunks via copyIntoBuffer(), amortizing OOC overhead across
 * thousands of elements per read instead of per-element random access.
 *
 * **Distance computation (canMerge)**: When checking if two adjacent grid cells should
 * merge, pairwise distances between all points in both cells are computed. The Direct
 * variant uses operator[] to index directly into the full input array by tuple index.
 * The Scanline variant instead uses readGridCellCoords() to bulk-read all coordinate
 * data for each grid cell into a local buffer, then performs pairwise distances entirely
 * in memory. Memory cost is O(gridCellSize * dims) per canMerge call, not O(n).
 *
 * **Labeling**: Both variants use setValue() for writing cluster IDs, which is acceptable
 * since labeling is a single sequential pass.
 *
 * Selected by DispatchAlgorithm when any input array is backed by out-of-core storage.
 *
 * @see DBSCANDirect for the in-core-optimized alternative.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism that selects between them.
 */
class SIMPLNXCORE_EXPORT DBSCANScanline
{
public:
  /**
   * @brief Constructs the out-of-core algorithm with all resources it needs.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  DBSCANScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANScanline() noexcept;

  DBSCANScanline(const DBSCANScanline&) = delete;
  DBSCANScanline(DBSCANScanline&&) noexcept = delete;
  DBSCANScanline& operator=(const DBSCANScanline&) = delete;
  DBSCANScanline& operator=(DBSCANScanline&&) noexcept = delete;

  /**
   * @brief Executes the OOC-optimized DBSCAN clustering: grid construction, clustering, labeling.
   * @return Result<> with any errors encountered during execution
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                   ///< Reference to the DataStructure containing all arrays
  const DBSCANInputValues* m_InputValues = nullptr; ///< Non-owning pointer to input parameters
  const std::atomic_bool& m_ShouldCancel;           ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;  ///< Message handler for progress updates
};

} // namespace nx::core

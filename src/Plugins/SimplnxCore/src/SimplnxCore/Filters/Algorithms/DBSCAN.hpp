#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include <random>

namespace nx::core
{
/**
 * @struct DBSCANInputValues
 * @brief Input parameter bundle for the DBSCAN algorithm.
 *
 * Aggregates all DataPaths and configuration values needed by both the in-core
 * (Direct) and out-of-core (Scanline) variants of DBSCAN clustering.
 */
struct SIMPLNXCORE_EXPORT DBSCANInputValues
{
  DataPath ClusteringArrayPath;                    ///< Input array containing 2D or 3D coordinate data to cluster
  DataPath MaskArrayPath;                          ///< Input Bool/UInt8 mask; false elements become outliers (cluster 0)
  DataPath FeatureIdsArrayPath;                    ///< Output Int32 array storing per-element cluster assignments
  float32 Epsilon;                                 ///< Maximum distance for density-connectivity; also determines grid cell size
  int32 MinPoints;                                 ///< Minimum points in a grid cell for it to be a "core" cell
  ClusterUtilities::DistanceMetric DistanceMetric; ///< Distance metric used for canMerge checks between grid cells
  DataPath FeatureAM;                              ///< Output Attribute Matrix resized to (maxCluster + 1) after clustering
  ChoicesParameter::ValueType ParseOrder;          ///< Order for processing core grids: LowDensityFirst, Random, or SeededRandom
  std::mt19937_64::result_type Seed;               ///< Random seed for reproducible parse order (SeededRandom mode)
};

/**
 * @class DBSCAN
 * @brief Dispatcher algorithm for grid-based DBSCAN density clustering.
 *
 * Implements a modified DBSCAN algorithm based on Grid-based DBSCAN (GDCF) from
 * Boonchoo et al. 2019. Data points are binned into a regular grid with cell side
 * length epsilon / sqrt(dims). Grid cells with >= minPoints are "core" cells that
 * form initial clusters. Adjacent grid cells are merged if any pair of points across
 * them has distance < epsilon.
 *
 * This class acts as a thin dispatcher that selects between two concrete implementations:
 *
 * - **DBSCANDirect** (in-core): Uses per-element operator[] access for grid construction
 *   and direct random access for canMerge distance checks. Optimal when all arrays
 *   reside in memory.
 *
 * - **DBSCANScanline** (out-of-core / OOC): Uses chunked copyIntoBuffer() bulk I/O
 *   for grid construction (bounds detection, binning, cell filling). For canMerge
 *   distance checks, reads grid cell coordinate data on-demand into local buffers
 *   instead of random per-element access across the full array.
 *
 * The dispatch decision is made by DispatchAlgorithm<Direct, Scanline>() in
 * AlgorithmDispatch.hpp, which checks whether any input IDataArray uses OOC storage.
 *
 * @see DBSCANDirect
 * @see DBSCANScanline
 * @see AlgorithmDispatch.hpp
 */
class SIMPLNXCORE_EXPORT DBSCAN
{
public:
  /**
   * @brief Constructs the dispatcher with all resources needed by either algorithm variant.
   * @param dataStructure The DataStructure containing input/output arrays
   * @param mesgHandler Message handler for progress reporting
   * @param shouldCancel Atomic flag checked periodically to support user cancellation
   * @param inputValues Non-owning pointer to the parameter bundle
   */
  DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues);
  ~DBSCAN() noexcept;

  DBSCAN(const DBSCAN&) = delete;
  DBSCAN(DBSCAN&&) noexcept = delete;
  DBSCAN& operator=(const DBSCAN&) = delete;
  DBSCAN& operator=(DBSCAN&&) noexcept = delete;

  /**
   * @enum ParseOrder
   * @brief Controls the order in which core grid cells are processed during initial clustering.
   */
  enum ParseOrder
  {
    LowDensityFirst, ///< Process lower-density core grids first (deterministic, typically fastest)
    Random,          ///< Process in non-deterministic random order (time-based seed)
    SeededRandom     ///< Process in deterministic random order (user-supplied seed)
  };

  /**
   * @brief Dispatches to the Direct or Scanline algorithm based on storage type.
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

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeEuclideanDistMapInputValues
 * @brief Holds all user-configured parameters for the ComputeEuclideanDistMap algorithm.
 */
struct SIMPLNXCORE_EXPORT ComputeEuclideanDistMapInputValues
{
  bool CalcManhattanDist;        ///< If true, output Manhattan (city-block) distances as int32; otherwise Euclidean as float32.
  bool DoBoundaries;             ///< Compute distance to nearest grain boundary (2+ unique neighbors).
  bool DoTripleLines;            ///< Compute distance to nearest triple line (3+ unique neighbors).
  bool DoQuadPoints;             ///< Compute distance to nearest quadruple point (4+ unique neighbors).
  DataPath FeatureIdsArrayPath;  ///< Per-cell Feature ID array (int32).
  DataPath GBDistancesArrayPath; ///< Output: grain boundary distance map.
  DataPath TJDistancesArrayPath; ///< Output: triple junction distance map.
  DataPath QPDistancesArrayPath; ///< Output: quadruple point distance map.
  DataPath InputImageGeometry;   ///< Path to the ImageGeom.
};

/**
 * @class ComputeEuclideanDistMap
 * @brief Computes distance maps from each voxel to the nearest grain boundary,
 * triple junction, and/or quadruple point using iterative neighbor propagation
 * followed by optional Euclidean distance correction.
 *
 * The algorithm first identifies boundary voxels by checking each voxel's 6 face
 * neighbors for different Feature IDs. Voxels with 2+ distinct neighbors are grain
 * boundaries, 3+ are triple lines, 4+ are quadruple points. It then propagates
 * distances outward using iterative "city-block" expansion and optionally converts
 * to true Euclidean distances.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The original implementation accessed FeatureIds and distance DataStores through
 * per-element virtual dispatch in multiple passes (boundary identification, iterative
 * propagation, final distance write-back). For OOC data, this caused severe chunk
 * thrashing across all passes.
 *
 * The optimized implementation:
 * 1. Bulk-reads the entire FeatureIds array into a local std::vector via
 *    copyIntoBuffer() at the start of FindDistanceMap().
 * 2. Bulk-reads each distance store into local buffers after the initial fill(-1).
 * 3. All boundary identification and distance propagation operates on local buffers.
 * 4. Each ComputeDistanceMapImpl worker receives raw pointers to the pre-loaded
 *    buffers instead of DataStore references, eliminating virtual dispatch.
 * 5. Results are written back via a single copyFromBuffer() call per map.
 */
class SIMPLNXCORE_EXPORT ComputeEuclideanDistMap
{
public:
  ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeEuclideanDistMapInputValues* inputValues);
  ~ComputeEuclideanDistMap() noexcept;

  ComputeEuclideanDistMap(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap(ComputeEuclideanDistMap&&) noexcept = delete;
  ComputeEuclideanDistMap& operator=(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap& operator=(ComputeEuclideanDistMap&&) noexcept = delete;

  using EnumType = uint32_t;

  /**
   * @enum MapType
   * @brief Identifies which type of distance map a ComputeDistanceMapImpl instance computes.
   */
  enum class MapType : EnumType
  {
    FeatureBoundary = 0, ///< Distance to nearest grain boundary (2+ unique neighbors).
    TripleJunction = 1,  ///< Distance to nearest triple junction (3+ unique neighbors).
    QuadPoint = 2,       ///< Distance to nearest quadruple point (4+ unique neighbors).
  };

  /**
   * @brief Executes the distance map computation, dispatching int32 (Manhattan) or float32 (Euclidean).
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return const reference to the atomic cancellation boolean.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                    ///< Reference to the DataStructure.
  const ComputeEuclideanDistMapInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                            ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                   ///< Message handler for progress.
};

} // namespace nx::core

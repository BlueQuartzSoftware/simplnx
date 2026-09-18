#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeEuclideanDistMapInputValues
 * @brief Stores filter values for a distance-map execution.
 */
struct SIMPLNXCORE_EXPORT ComputeEuclideanDistMapInputValues
{
  bool CalcManhattanDist; ///< True to create int32 city-block maps. False creates float32 Euclidean maps.
  // These flags select seeds with one, two, or three distinct neighboring Feature IDs.
  bool DoBoundaries;
  bool DoTripleLines;
  bool DoQuadPoints;
  DataPath FeatureIdsArrayPath;
  DataPath GBDistancesArrayPath;
  DataPath TJDistancesArrayPath;
  DataPath QPDistancesArrayPath;
  DataPath InputImageGeometry;
};

/**
 * @class ComputeEuclideanDistMap
 * @brief Computes requested boundary, triple-line, and quad-point distance maps.
 *
 * A seed pass examines six face neighbors. One, two, and three distinct neighbor Feature IDs
 * create the requested seed types. Manhattan propagation supplies optional Euclidean correction.
 *
 * The normal dispatcher selects scanline execution when any required input or output uses
 * out-of-core storage. Resident stores also use scanline execution except when all maps and blocked
 * cells select the measured direct-path exception. Direct execution allocates full-volume buffers.
 * It is not an out-of-core memory bound.
 *
 * Scanline execution uses rolling Z-slice buffers. Euclidean mode stores nearest-seed state in a
 * temporary DataStore that follows the active storage policy. Direct workers calculate from local
 * buffers. This specialization does not establish generic DataArray or DataStore thread safety.
 */
class SIMPLNXCORE_EXPORT ComputeEuclideanDistMap
{
public:
  /**
   * @brief Initializes the distance-map algorithm.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and output maps.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects map types and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeEuclideanDistMapInputValues* inputValues);
  /**
   * @brief Destroys the distance-map algorithm.
   */
  ~ComputeEuclideanDistMap() noexcept;

  ComputeEuclideanDistMap(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap(ComputeEuclideanDistMap&&) noexcept = delete;
  ComputeEuclideanDistMap& operator=(const ComputeEuclideanDistMap&) = delete;
  ComputeEuclideanDistMap& operator=(ComputeEuclideanDistMap&&) noexcept = delete;

  /**
   * @brief Defines the numeric type for map selectors.
   */
  using EnumType = uint32_t;

  /**
   * @enum MapType
   * @brief Identifies the seed type for one distance map.
   */
  enum class MapType : EnumType
  {
    FeatureBoundary = 0, ///< Uses seeds with one distinct neighboring Feature ID.
    TripleJunction = 1,  ///< Uses seeds with two distinct neighboring Feature IDs.
    QuadPoint = 2,       ///< Uses seeds with three distinct neighboring Feature IDs.
  };

  /**
   * @brief Computes requested distance maps.
   * @return Success.
   *
   * Both implementations return success when a cancellation checkpoint observes the signal. Direct
   * tasks run to completion after they start. Scanline execution preserves completed map ranges.
   *
   * Current bulk-I/O Result values are not inspected. A storage failure can leave partial maps and
   * still return success.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeEuclideanDistMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

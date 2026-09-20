#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct ComputeFeatureNeighborsInputValues;

/**
 * @class ComputeFeatureNeighborsDirect
 * @brief Computes feature neighbors with direct resident-store access.
 *
 * Boundary cells validate present faces. The 3D interior avoids that validity branch. Each face uses
 * its physical area instead of a uniform area. Float64 map values accumulate shared area without
 * Kahan compensators, which would add state for every neighbor relationship. The normal dispatcher
 * selects this path for resident Feature IDs. A forced direct out-of-core run can perform per-element
 * store access.
 *
 * @see ComputeFeatureNeighborsScanline.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighborsDirect
{
public:
  /**
   * @brief Initializes the direct feature-neighbor algorithm.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation during the 3D interior sweep.
   * @param inputValues Selects outputs and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureNeighborsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureNeighborsInputValues* inputValues);
  /**
   * @brief Destroys the direct feature-neighbor algorithm.
   */
  ~ComputeFeatureNeighborsDirect() noexcept;

  ComputeFeatureNeighborsDirect(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect(ComputeFeatureNeighborsDirect&&) noexcept = delete;
  ComputeFeatureNeighborsDirect& operator=(const ComputeFeatureNeighborsDirect&) = delete;
  ComputeFeatureNeighborsDirect& operator=(ComputeFeatureNeighborsDirect&&) noexcept = delete;

  /**
   * @brief Computes direct feature-neighbor output.
   * @return Success, or a Feature ID range or output-array error.
   *
   * Cancellation is checked only during the 3D interior sweep. Boundary and output phases do not
   * inspect cancellation. A cancellation return can preserve partial BoundaryCells output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

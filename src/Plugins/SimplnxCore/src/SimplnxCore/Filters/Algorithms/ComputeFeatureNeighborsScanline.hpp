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
 * @class ComputeFeatureNeighborsScanline
 * @brief Computes feature neighbors with a rolling Feature ID window.
 *
 * Three Feature ID slices resolve all face neighbors with sequential bulk reads. BoundaryCells uses
 * one matching output slice. This requires 12 bytes per XY cell, plus 1 byte when BoundaryCells is
 * selected. Feature-neighbor maps retain all observed relationships in memory. Per-face physical
 * areas preserve anisotropic ImageGeom surface area.
 *
 * Dispatch uses Feature IDs only. Optional surface flags use direct feature-level writes. Current
 * bulk-I/O Result values are not inspected. A storage failure can leave partial BoundaryCells output
 * while the method returns success.
 *
 * @see ComputeFeatureNeighborsDirect.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighborsScanline
{
public:
  /**
   * @brief Initializes the scanline feature-neighbor algorithm.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between Z slices.
   * @param inputValues Selects outputs and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureNeighborsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  const ComputeFeatureNeighborsInputValues* inputValues);
  /**
   * @brief Destroys the scanline feature-neighbor algorithm.
   */
  ~ComputeFeatureNeighborsScanline() noexcept;

  ComputeFeatureNeighborsScanline(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline(ComputeFeatureNeighborsScanline&&) noexcept = delete;
  ComputeFeatureNeighborsScanline& operator=(const ComputeFeatureNeighborsScanline&) = delete;
  ComputeFeatureNeighborsScanline& operator=(ComputeFeatureNeighborsScanline&&) noexcept = delete;

  /**
   * @brief Computes feature-neighbor output with rolling slices.
   * @return Success, or a Feature ID range error.
   *
   * When a Z-slice checkpoint observes cancellation, the method returns success. Completed
   * BoundaryCells slices remain written. Later slices and neighbor-list output are not written.
   *
   * The maximum Feature ID check occurs after slice processing. A range error can therefore leave
   * BoundaryCells slices and surface flags changed while neighbor-list output is not written.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

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

struct ComputeFeatureBoundsInputValues;

/**
 * @class ComputeFeatureBoundsScanline
 * @brief Computes feature bounds with bounded Feature ID transfers.
 *
 * The implementation reads 65,536 Feature IDs per bulk transfer and retains six float32 values per
 * feature. Node-geometry vertex and connectivity stores remain direct element accesses. Feature ID
 * storage is the dispatcher witness; this implementation does not provide a generic OOC geometry
 * access guarantee.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureBoundsScanline
{
public:
  /**
   * @brief Initializes the scanline feature-bound algorithm.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects output layout and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureBoundsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeFeatureBoundsInputValues* inputValues);
  /**
   * @brief Destroys the scanline feature-bound algorithm.
   */
  ~ComputeFeatureBoundsScanline() noexcept;

  ComputeFeatureBoundsScanline(const ComputeFeatureBoundsScanline&) = delete;
  ComputeFeatureBoundsScanline(ComputeFeatureBoundsScanline&&) noexcept = delete;
  ComputeFeatureBoundsScanline& operator=(const ComputeFeatureBoundsScanline&) = delete;
  ComputeFeatureBoundsScanline& operator=(ComputeFeatureBoundsScanline&&) noexcept = delete;

  /**
   * @brief Computes feature bounds with bounded Feature ID reads.
   * @return Success, or a Feature ID bulk-I/O, geometry, or feature-sizing error.
   *
   * Cancellation returns success when a batch or phase checkpoint observes the signal. Bounds and
   * edge-geometry writes do not begin after that checkpoint. Cancellation during those writes is not checked.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureBoundsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

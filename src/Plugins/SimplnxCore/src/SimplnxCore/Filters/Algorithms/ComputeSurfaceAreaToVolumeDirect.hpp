#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

/**
 * @class ComputeSurfaceAreaToVolumeDirect
 * @brief Computes surface metrics from in-memory Feature Id values.
 *
 * Flat neighbor offsets are efficient for contiguous data. The Scanline variant
 * replaces disk-backed neighbor reads with sequential slice I/O. A local feature
 * accumulator prevents repeated output-store read-modify-write operations.
 *
 * @see ComputeSurfaceAreaToVolumeScanline for the OOC-optimized variant.
 * @see ComputeSurfaceAreaToVolume for the dispatcher.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeDirect
{
public:
  /**
   * @brief Creates an in-memory surface-metric algorithm.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeSurfaceAreaToVolumeDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  /**
   * @brief Destroys the non-owning in-memory algorithm.
   */
  ~ComputeSurfaceAreaToVolumeDirect() noexcept;

  ComputeSurfaceAreaToVolumeDirect(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(const ComputeSurfaceAreaToVolumeDirect&) = delete;
  ComputeSurfaceAreaToVolumeDirect& operator=(ComputeSurfaceAreaToVolumeDirect&&) noexcept = delete;

  /**
   * @brief Calculates surface-area-to-volume ratio and optional sphericity.
   * @return Error from Feature Id validation, or success after cancellation.
   *
   * Cancellation during the cell scan leaves metric outputs unchanged.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

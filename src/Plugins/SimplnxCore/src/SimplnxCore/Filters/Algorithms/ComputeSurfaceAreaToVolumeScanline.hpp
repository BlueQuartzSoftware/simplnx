#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceAreaToVolumeInputValues;

/**
 * @class ComputeSurfaceAreaToVolumeScanline
 * @brief Computes surface metrics with a three-slice Feature Id window.
 *
 * The window converts disk-backed neighbor reads to sequential slice I/O. Memory
 * is three cell-scale slices plus feature-scale caches. Feature caches can be
 * large when the feature count approaches the cell count.
 *
 * @see ComputeSurfaceAreaToVolumeDirect for the in-core variant.
 * @see ComputeSurfaceAreaToVolume for the dispatcher.
 * @see DispatchAlgorithm for the selection mechanism.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceAreaToVolumeScanline
{
public:
  /**
   * @brief Creates a bulk-I/O surface-metric algorithm.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeSurfaceAreaToVolumeScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeSurfaceAreaToVolumeInputValues* inputValues);
  /**
   * @brief Destroys the non-owning bulk-I/O algorithm.
   */
  ~ComputeSurfaceAreaToVolumeScanline() noexcept;

  ComputeSurfaceAreaToVolumeScanline(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(const ComputeSurfaceAreaToVolumeScanline&) = delete;
  ComputeSurfaceAreaToVolumeScanline& operator=(ComputeSurfaceAreaToVolumeScanline&&) noexcept = delete;

  /**
   * @brief Calculates surface metrics with bulk I/O.
   * @return Error from Feature Id validation, or success after cancellation.
   *
   * Cancellation during the cell scan leaves metric outputs unchanged. Current
   * bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

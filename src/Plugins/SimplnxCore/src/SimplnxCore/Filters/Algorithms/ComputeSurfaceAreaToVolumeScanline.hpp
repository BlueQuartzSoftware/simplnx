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
 * The window converts disk-backed neighbor reads to sequential slice I/O. The
 * three label slices are released before count and metric batches are allocated.
 * Let F be the number of feature tuples and S the number of cells per XY slice.
 * For F > 0, these buffers require 8*F + max(12*S, 8*B) bytes.
 * The batch capacity is B = min(65536, max(1, floor(F/2))).
 * The 8*F-byte area accumulator can still be large. This estimate excludes
 * stores, caches, allocator overhead, and the separate Feature Id validation.
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
   * @return Error from Feature Id validation or bulk I/O; success on completion or cancellation.
   *
   * At each slice start, cancellation stops processing before metric writes.
   * A bulk-I/O error stops processing. A later count read or metric write error
   * can leave earlier batches written. Output updates are not transactional.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceAreaToVolumeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

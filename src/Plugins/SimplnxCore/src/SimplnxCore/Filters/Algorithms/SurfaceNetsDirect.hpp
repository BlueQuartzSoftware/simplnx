#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsDirect
 * @brief Builds Surface Nets output through the resident MMSurfaceNet library.
 *
 * MMSurfaceNet retains one cell for each padded grid position and reads Feature
 * IDs by value. Memory and random access are proportional to padded volume.
 * A forced direct run on disk-backed data can cause repeated chunk access.
 *
 * Classification and optional relaxation do not inspect cancellation. Later
 * vertex, counting, and face-generation loops check between vertices. Selected
 * tuple transfers use per-value store access and cannot report storage errors.
 * Optional winding repair creates resident connectivity before it runs.
 *
 * @see SurfaceNetsScanline for external padded-cell records and bulk output.
 */
class SIMPLNXCORE_EXPORT SurfaceNetsDirect
{
public:
  /**
   * @brief Initializes the resident Surface Nets implementation.
   * @param dataStructure Contains input and output objects.
   * @param mesgHandler Receives winding messages.
   * @param shouldCancel Signals cancellation after classification and between later vertices.
   * @param inputValues Selects smoothing, winding, transfers, and paths.
   * @pre All arguments outlive this executor.
   */
  SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  /**
   * @brief Destroys the resident Surface Nets implementation.
   */
  ~SurfaceNetsDirect() noexcept;

  SurfaceNetsDirect(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect(SurfaceNetsDirect&&) noexcept = delete;
  SurfaceNetsDirect& operator=(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect& operator=(SurfaceNetsDirect&&) noexcept = delete;

  /**
   * @brief Classifies cells, writes mesh output, and optionally repairs winding.
   * @return Cell-map allocation, connectivity, or winding result.
   *
   * Cancellation returns success without rollback. Tuple-transfer storage
   * failures cannot be returned by the direct per-value interface.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

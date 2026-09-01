#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesDirect
 * @brief Identifies surface features from in-memory Feature Id values.
 *
 * The algorithm uses four or six face neighbors for 2D or 3D geometries. The
 * Scanline variant replaces disk-backed neighbor reads with sequential slice I/O.
 *
 * @see ComputeSurfaceFeaturesScanline for the OOC-optimized variant.
 * @see ComputeSurfaceFeatures for the dispatcher.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesDirect
{
public:
  /**
   * @brief Creates an in-memory surface-feature algorithm.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeSurfaceFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeSurfaceFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning in-memory algorithm.
   */
  ~ComputeSurfaceFeaturesDirect() noexcept;

  ComputeSurfaceFeaturesDirect(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect(ComputeSurfaceFeaturesDirect&&) noexcept = delete;
  ComputeSurfaceFeaturesDirect& operator=(const ComputeSurfaceFeaturesDirect&) = delete;
  ComputeSurfaceFeaturesDirect& operator=(ComputeSurfaceFeaturesDirect&&) noexcept = delete;

  /**
   * @brief Labels surface features.
   * @return Error from validation or unsupported dimensionality, or success after cancellation.
   *
   * Cancellation can retain labels assigned before the method returns.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

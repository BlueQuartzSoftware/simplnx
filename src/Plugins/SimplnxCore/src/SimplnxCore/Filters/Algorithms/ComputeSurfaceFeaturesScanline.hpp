#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeSurfaceFeaturesInputValues;

/**
 * @class ComputeSurfaceFeaturesScanline
 * @brief Identifies surface features with a three-slice Feature Id window.
 *
 * The window converts disk-backed neighbor reads to sequential slice I/O. A
 * unified loop remaps 2D geometry coordinates without losing sequential access.
 * Memory is three cell-scale slices plus a feature-scale output cache.
 *
 * @see ComputeSurfaceFeaturesDirect for the in-core variant.
 * @see ComputeSurfaceFeatures for the dispatcher.
 * @see DispatchAlgorithm for the selection mechanism.
 */
class SIMPLNXCORE_EXPORT ComputeSurfaceFeaturesScanline
{
public:
  /**
   * @brief Creates a bulk-I/O surface-feature algorithm.
   * @param dataStructure Provides the selected arrays and geometry.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later slices when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the algorithm lifetime.
   */
  ComputeSurfaceFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeSurfaceFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning bulk-I/O algorithm.
   */
  ~ComputeSurfaceFeaturesScanline() noexcept;

  ComputeSurfaceFeaturesScanline(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline(ComputeSurfaceFeaturesScanline&&) noexcept = delete;
  ComputeSurfaceFeaturesScanline& operator=(const ComputeSurfaceFeaturesScanline&) = delete;
  ComputeSurfaceFeaturesScanline& operator=(ComputeSurfaceFeaturesScanline&&) noexcept = delete;

  /**
   * @brief Labels surface features with bulk I/O.
   * @return Error from validation or unsupported dimensionality, or success after cancellation.
   *
   * Cancellation leaves the output unchanged because write-back follows the cell
   * scan. Current bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeSurfaceFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

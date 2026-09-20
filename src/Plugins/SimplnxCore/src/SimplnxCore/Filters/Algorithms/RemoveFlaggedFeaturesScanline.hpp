#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct RemoveFlaggedFeaturesInputValues;

/**
 * @class RemoveFlaggedFeaturesScanline
 * @brief Extracts or removes flagged features through bounded Z-slice I/O.
 *
 * Three Feature-ID slices preserve one iteration's vote state. Two mark slices
 * defer each write until the read frontier passes the destination slice.
 * This order prevents earlier writes from changing later votes.
 *
 * Each companion array transfers one destination slice and at most three source
 * slices. Fill scratch therefore scales with slice area and array component width.
 *
 * @see RemoveFlaggedFeaturesDirect
 * @see RemoveFlaggedFeatures
 */
class SIMPLNXCORE_EXPORT RemoveFlaggedFeaturesScanline
{
public:
  /**
   * @brief Creates a scanline flagged-feature algorithm.
   * @param dataStructure Provides source geometry, feature data, and outputs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops before later extraction, slices, or chunks when true.
   * @param inputValues Specifies validated settings and paths. The caller must keep
   * this object alive for the algorithm lifetime.
   */
  RemoveFlaggedFeaturesScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const RemoveFlaggedFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning algorithm.
   */
  ~RemoveFlaggedFeaturesScanline() noexcept;

  RemoveFlaggedFeaturesScanline(const RemoveFlaggedFeaturesScanline&) = delete;
  RemoveFlaggedFeaturesScanline(RemoveFlaggedFeaturesScanline&&) noexcept = delete;
  RemoveFlaggedFeaturesScanline& operator=(const RemoveFlaggedFeaturesScanline&) = delete;
  RemoveFlaggedFeaturesScanline& operator=(RemoveFlaggedFeaturesScanline&&) noexcept = delete;

  /**
   * @brief Extracts or removes selected features through bounded I/O.
   * @return First reported removal or I/O error, or success after cancellation.
   *
   * Delegated preflight failures throw. Delegated execute failures are not inspected.
   * Cancellation and errors can retain output slices, geometries, or modified IDs.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

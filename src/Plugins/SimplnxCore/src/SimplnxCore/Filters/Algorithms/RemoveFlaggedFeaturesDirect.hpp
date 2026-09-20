#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct RemoveFlaggedFeaturesInputValues;

/**
 * @class RemoveFlaggedFeaturesDirect
 * @brief Extracts or removes flagged features through resident direct access.
 *
 * Gap filling repeats a six-face-neighbor vote until no removed voxel remains.
 * Direct tuple access suits resident arrays. Disk-backed arrays use the scanline variant.
 *
 * @see RemoveFlaggedFeaturesScanline
 * @see RemoveFlaggedFeatures
 */
class SIMPLNXCORE_EXPORT RemoveFlaggedFeaturesDirect
{
public:
  /**
   * @brief Creates a direct flagged-feature algorithm.
   * @param dataStructure Provides source geometry, feature data, and outputs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later extraction or removal work when true.
   * @param inputValues Specifies validated settings and paths. The caller must keep
   * this object alive for the algorithm lifetime.
   */
  RemoveFlaggedFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const RemoveFlaggedFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning algorithm.
   */
  ~RemoveFlaggedFeaturesDirect() noexcept;

  RemoveFlaggedFeaturesDirect(const RemoveFlaggedFeaturesDirect&) = delete;
  RemoveFlaggedFeaturesDirect(RemoveFlaggedFeaturesDirect&&) noexcept = delete;
  RemoveFlaggedFeaturesDirect& operator=(const RemoveFlaggedFeaturesDirect&) = delete;
  RemoveFlaggedFeaturesDirect& operator=(RemoveFlaggedFeaturesDirect&&) noexcept = delete;

  /**
   * @brief Extracts or removes selected features through direct access.
   * @return First reported removal error, or success after cancellation.
   *
   * Delegated preflight failures throw. Delegated execute failures are not inspected.
   * Cancellation and errors can retain extracted geometries or modified source arrays.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SilhouetteInputValues;

/**
 * @class SilhouetteScanline
 * @brief Computes exact silhouette scores with bounded tiles and checked bulk I/O.
 *
 * Three passes discover sparse positive Feature IDs, count enabled tuples, and
 * perform the all-pairs calculation. Pairwise input tiles are reread for each
 * outer tile. Memory is O(128 times (component count plus feature count)), not
 * tuple count. The repeated reads preserve deterministic tuple order and exact
 * Direct-path mathematics with sequential disk access.
 *
 * Feature zero is a valid own cluster but is not an alternate cluster for b.
 * Negative Feature IDs return an error before output begins. Cancellation in
 * discovery or counting leaves output unchanged. Cancellation during pairwise
 * work preserves completed output tiles.
 *
 * @see SilhouetteDirect for the resident distance-table implementation.
 */
class SIMPLNXCORE_EXPORT SilhouetteScanline
{
public:
  /**
   * @brief Initializes the bulk-I/O silhouette implementation.
   * @param dataStructure Contains input, Feature ID, mask, and output arrays.
   * @param messageHandler Preserves the common interface but receives no messages.
   * @param shouldCancel Signals cancellation between passes and tiles.
   * @param inputValues Selects metric and array paths.
   * @pre All arguments outlive this executor.
   */
  SilhouetteScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const SilhouetteInputValues* inputValues);
  /**
   * @brief Destroys the bulk-I/O silhouette implementation.
   */
  ~SilhouetteScanline() noexcept;

  SilhouetteScanline(const SilhouetteScanline&) = delete;
  SilhouetteScanline(SilhouetteScanline&&) noexcept = delete;
  SilhouetteScanline& operator=(const SilhouetteScanline&) = delete;
  SilhouetteScanline& operator=(SilhouetteScanline&&) noexcept = delete;

  /**
   * @brief Discovers clusters and writes tiled silhouette scores.
   * @return Bulk-I/O, mask-type, Feature ID, or size-overflow result.
   * @pre Participating arrays have equal tuple counts.
   *
   * Cancellation returns success. Scores from completed outer tiles remain.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const SilhouetteInputValues* m_InputValues = nullptr;
};
} // namespace nx::core

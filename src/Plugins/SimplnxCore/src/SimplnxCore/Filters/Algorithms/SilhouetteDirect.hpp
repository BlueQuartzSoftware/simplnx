#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SilhouetteInputValues;

/**
 * @class SilhouetteDirect
 * @brief Computes silhouette scores with resident direct array access.
 *
 * The algorithm retains N by (K + 1) distance values, where N is tuple count and
 * K is the number of distinct Feature IDs. It also retains three N-value vectors.
 * Direct indexing is efficient for resident arrays but causes random access when
 * a storage override forces this path on disk-backed data.
 *
 * Feature IDs index the distance table directly. They must be nonnegative, and
 * the maximum ID must not exceed the number of distinct IDs. The implementation
 * does not validate this condition. It does not inspect cancellation.
 *
 * @see SilhouetteScanline for sparse-ID support and bounded tuple tiles.
 */
class SIMPLNXCORE_EXPORT SilhouetteDirect
{
public:
  /**
   * @brief Initializes the resident silhouette implementation.
   * @param dataStructure Contains input, Feature ID, mask, and output arrays.
   * @param messageHandler Preserves the common dispatcher constructor signature.
   * @param shouldCancel Preserves the common interface but is not inspected.
   * @param inputValues Selects metric and array paths.
   * @pre dataStructure and inputValues outlive this executor.
   */
  SilhouetteDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const SilhouetteInputValues* inputValues);
  /**
   * @brief Destroys the resident silhouette implementation.
   */
  ~SilhouetteDirect() noexcept;

  SilhouetteDirect(const SilhouetteDirect&) = delete;
  SilhouetteDirect(SilhouetteDirect&&) noexcept = delete;
  SilhouetteDirect& operator=(const SilhouetteDirect&) = delete;
  SilhouetteDirect& operator=(SilhouetteDirect&&) noexcept = delete;

  /**
   * @brief Computes all enabled tuple scores with a resident distance table.
   * @return Mask-instantiation result, or success.
   * @pre Participating arrays have equal tuple counts.
   * @pre Feature IDs are nonnegative and their maximum does not exceed the distinct-ID count.
   *
   * Masked tuples receive zero. The method runs to completion after it starts.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const SilhouetteInputValues* m_InputValues = nullptr;
};
} // namespace nx::core

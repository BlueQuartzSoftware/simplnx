#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsDirect
 * @brief Computes K-Medoids clusters with direct element access.
 *
 * Direct access is efficient for resident arrays. A forced direct path uses the
 * same element access for out-of-core arrays and can cause chunk thrashing.
 * Medoid optimization evaluates every cluster member against its peers.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsDirect
{
public:
  /**
   * @brief Initializes direct K-Medoids clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMedoidsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsDirect() noexcept;

  ComputeKMedoidsDirect(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect(ComputeKMedoidsDirect&&) noexcept = delete;
  ComputeKMedoidsDirect& operator=(const ComputeKMedoidsDirect&) = delete;
  ComputeKMedoidsDirect& operator=(ComputeKMedoidsDirect&&) noexcept = delete;

  /**
   * @brief Executes K-Medoids with direct element access.
   * @return Success, or an empty-input, invalid-mask, empty-mask, or mask-read error.
   * @pre The component count is positive and fits in Int32.
   * @pre The cluster count fits Int32 and output arrays have compatible shapes.
   *
   * This path does not validate these cluster and shape preconditions.
   * Cancellation returns success and can leave partial assignments or medoids.
   */
  Result<> operator()();

  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KMedoidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

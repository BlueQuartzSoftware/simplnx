#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct KMedoidsInputValues;

/**
 * @class ComputeKMedoidsScanline
 * @brief Computes K-Medoids clusters with bounded bulk I/O.
 *
 * The path caches cluster-scale medoid state. Input values, masks, assignments,
 * candidates, and comparison targets use fixed 4,096-tuple tiles. Candidate
 * costs retain tuple order and strict tie comparisons from the direct path.
 */
class SIMPLNXCORE_EXPORT ComputeKMedoidsScanline
{
public:
  /**
   * @brief Initializes scanline K-Medoids clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation between bounded operations.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMedoidsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const KMedoidsInputValues* inputValues);
  ~ComputeKMedoidsScanline() noexcept;

  ComputeKMedoidsScanline(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline(ComputeKMedoidsScanline&&) noexcept = delete;
  ComputeKMedoidsScanline& operator=(const ComputeKMedoidsScanline&) = delete;
  ComputeKMedoidsScanline& operator=(ComputeKMedoidsScanline&&) noexcept = delete;

  /**
   * @brief Executes K-Medoids with bounded bulk transfers.
   * @return Success, or a mask, shape, overflow, recovery, or transfer error.
   *
   * Cancellation returns success. Completed assignment tiles and medoid writes
   * from earlier phases remain in the output arrays.
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

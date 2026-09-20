#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct ComputeKMeansInputValues;

/**
 * @class ComputeKMeansDirect
 * @brief Computes K-Means clusters with direct element access.
 *
 * Direct access is efficient for resident arrays. A forced direct path uses the
 * same element access for out-of-core arrays and can cause chunk thrashing.
 * Centroid recomputation scans the input once for each component.
 */
class SIMPLNXCORE_EXPORT ComputeKMeansDirect
{
public:
  /**
   * @brief Initializes direct K-Means clustering.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives iteration messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ComputeKMeansDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ComputeKMeansInputValues* inputValues);
  ~ComputeKMeansDirect() noexcept;

  ComputeKMeansDirect(const ComputeKMeansDirect&) = delete;
  ComputeKMeansDirect(ComputeKMeansDirect&&) noexcept = delete;
  ComputeKMeansDirect& operator=(const ComputeKMeansDirect&) = delete;
  ComputeKMeansDirect& operator=(ComputeKMeansDirect&&) noexcept = delete;

  /**
   * @brief Executes K-Means with direct element access.
   * @return Success, or an invalid-mask or empty-mask error.
   * @pre The input has at least one tuple. Its component count is positive and fits in Int32.
   * @pre The cluster count is positive, fits in Int32, and output shapes are compatible.
   *
   * This path does not validate these shape preconditions. It does not check
   * cancellation during centroid selection. Later cancellation returns success
   * and can leave partial assignments or centroids.
   */
  Result<> operator()();

  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

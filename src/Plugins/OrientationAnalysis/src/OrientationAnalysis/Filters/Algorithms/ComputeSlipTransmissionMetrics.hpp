#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeSlipTransmissionMetricsInputValues
 * @brief Identifies the slip-transmission metric input and output arrays.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeSlipTransmissionMetricsInputValues
{
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath F1ListArrayName;
  DataPath F1sptListArrayName;
  DataPath F7ListArrayName;
  DataPath mPrimeListArrayName;
};

/**
 * @class ComputeSlipTransmissionMetrics
 * @brief Calculates slip-transmission metrics for neighboring features in cubic Phase 1.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeSlipTransmissionMetrics
{
public:
  /**
   * @brief Initializes the slip-transmission metric calculation.
   * @param dataStructure Provides the selected arrays and output neighbor lists.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and output neighbor lists.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this executor.
   */
  ComputeSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 ComputeSlipTransmissionMetricsInputValues* inputValues);

  /**
   * @brief Destroys the slip-transmission metric executor.
   */
  ~ComputeSlipTransmissionMetrics() noexcept;

  ComputeSlipTransmissionMetrics(const ComputeSlipTransmissionMetrics&) = delete;
  ComputeSlipTransmissionMetrics(ComputeSlipTransmissionMetrics&&) noexcept = delete;
  ComputeSlipTransmissionMetrics& operator=(const ComputeSlipTransmissionMetrics&) = delete;
  ComputeSlipTransmissionMetrics& operator=(ComputeSlipTransmissionMetrics&&) noexcept = delete;

  /**
   * @brief Calculates the metrics for each eligible feature-neighbor pair.
   * @return Success, a warning for non-cubic Phases, or an error for invalid Phase or Laue indices.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation signal used by this executor.
   * @return Reference that remains valid for the executor lifetime.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeSlipTransmissionMetricsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

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
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeSlipTransmissionMetrics
{
public:
  ComputeSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeSlipTransmissionMetricsInputValues* inputValues);
  ~ComputeSlipTransmissionMetrics() noexcept;

  ComputeSlipTransmissionMetrics(const ComputeSlipTransmissionMetrics&) = delete;
  ComputeSlipTransmissionMetrics(ComputeSlipTransmissionMetrics&&) noexcept = delete;
  ComputeSlipTransmissionMetrics& operator=(const ComputeSlipTransmissionMetrics&) = delete;
  ComputeSlipTransmissionMetrics& operator=(ComputeSlipTransmissionMetrics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeSlipTransmissionMetricsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

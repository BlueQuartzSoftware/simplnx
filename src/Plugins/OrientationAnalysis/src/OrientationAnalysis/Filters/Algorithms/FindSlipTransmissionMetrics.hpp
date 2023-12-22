#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindSlipTransmissionMetricsInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindSlipTransmissionMetrics
{
public:
  FindSlipTransmissionMetrics(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSlipTransmissionMetricsInputValues* inputValues);
  ~FindSlipTransmissionMetrics() noexcept;

  FindSlipTransmissionMetrics(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics(FindSlipTransmissionMetrics&&) noexcept = delete;
  FindSlipTransmissionMetrics& operator=(const FindSlipTransmissionMetrics&) = delete;
  FindSlipTransmissionMetrics& operator=(FindSlipTransmissionMetrics&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSlipTransmissionMetricsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

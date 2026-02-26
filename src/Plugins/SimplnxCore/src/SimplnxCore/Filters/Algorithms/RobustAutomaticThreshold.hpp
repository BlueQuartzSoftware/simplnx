#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RobustAutomaticThresholdInputValues
{
  DataObjectNameParameter::ValueType CreatedMaskName;
  ArraySelectionParameter::ValueType GradientArrayPath;
  ArraySelectionParameter::ValueType InputArrayPath;
};

/**
 * @class RobustAutomaticThreshold
 * @brief This algorithm implements support code for the RobustAutomaticThresholdFilter
 */

class SIMPLNXCORE_EXPORT RobustAutomaticThreshold
{
public:
  RobustAutomaticThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RobustAutomaticThresholdInputValues* inputValues);
  ~RobustAutomaticThreshold() noexcept;

  RobustAutomaticThreshold(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold(RobustAutomaticThreshold&&) noexcept = delete;
  RobustAutomaticThreshold& operator=(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold& operator=(RobustAutomaticThreshold&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RobustAutomaticThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

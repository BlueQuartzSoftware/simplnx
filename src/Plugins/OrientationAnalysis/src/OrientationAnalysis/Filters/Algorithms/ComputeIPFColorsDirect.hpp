#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeIPFColorsInputValues;

class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsDirect
{
public:
  ComputeIPFColorsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColorsDirect() noexcept;

  ComputeIPFColorsDirect(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect(ComputeIPFColorsDirect&&) = delete;
  ComputeIPFColorsDirect& operator=(const ComputeIPFColorsDirect&) = delete;
  ComputeIPFColorsDirect& operator=(ComputeIPFColorsDirect&&) = delete;

  Result<> operator()();

  void incrementPhaseWarningCount();
  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
  int32_t m_PhaseWarningCount = 0;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ComputeIPFColorsInputValues;

class ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsScanline
{
public:
  ComputeIPFColorsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, const ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColorsScanline() noexcept;

  ComputeIPFColorsScanline(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline(ComputeIPFColorsScanline&&) = delete;
  ComputeIPFColorsScanline& operator=(const ComputeIPFColorsScanline&) = delete;
  ComputeIPFColorsScanline& operator=(ComputeIPFColorsScanline&&) = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
};

} // namespace nx::core

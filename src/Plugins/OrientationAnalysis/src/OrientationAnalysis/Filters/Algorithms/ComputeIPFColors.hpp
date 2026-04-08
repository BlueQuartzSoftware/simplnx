#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsInputValues
{
  std::vector<float> referenceDirection;
  bool useGoodVoxels = false;
  DataPath goodVoxelsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellEulerAnglesArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath cellIpfColorsArrayPath;
};

class ORIENTATIONANALYSIS_EXPORT ComputeIPFColors
{
public:
  ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColors() noexcept;

  ComputeIPFColors(const ComputeIPFColors&) = delete;
  ComputeIPFColors(ComputeIPFColors&&) = delete;
  ComputeIPFColors& operator=(const ComputeIPFColors&) = delete;
  ComputeIPFColors& operator=(ComputeIPFColors&&) = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;
};

} // namespace nx::core

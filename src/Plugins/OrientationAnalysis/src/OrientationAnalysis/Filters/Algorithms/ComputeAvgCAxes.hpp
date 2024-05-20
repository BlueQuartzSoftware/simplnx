#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxesInputValues
{
  DataPath QuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellFeatureDataPath;
  DataPath AvgCAxesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class ComputeAvgCAxes
 * @brief This filter determines the average C-axis location of each Feature.
 */

class ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxes
{
public:
  ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues);
  ~ComputeAvgCAxes() noexcept;

  ComputeAvgCAxes(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes(ComputeAvgCAxes&&) noexcept = delete;
  ComputeAvgCAxes& operator=(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes& operator=(ComputeAvgCAxes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeAvgCAxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

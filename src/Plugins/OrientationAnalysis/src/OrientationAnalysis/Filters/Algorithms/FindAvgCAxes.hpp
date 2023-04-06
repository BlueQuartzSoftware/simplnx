#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindAvgCAxesInputValues
{
  DataPath QuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureDataPath;
  DataPath AvgCAxesArrayPath;
};

/**
 * @class FindAvgCAxes
 * @brief This filter determines the average C-axis location of each Feature.
 */

class ORIENTATIONANALYSIS_EXPORT FindAvgCAxes
{
public:
  FindAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindAvgCAxesInputValues* inputValues);
  ~FindAvgCAxes() noexcept;

  FindAvgCAxes(const FindAvgCAxes&) = delete;
  FindAvgCAxes(FindAvgCAxes&&) noexcept = delete;
  FindAvgCAxes& operator=(const FindAvgCAxes&) = delete;
  FindAvgCAxes& operator=(FindAvgCAxes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindAvgCAxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"


#include <vector>

namespace complex
{

/**
 * @brief The FindAvgOrientationsInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT FindAvgOrientationsInputValues
{
  DataPath cellFeatureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellQuatsArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath avgQuatsArrayPath;
  DataPath avgEulerAnglesArrayPath;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT FindAvgOrientations
{
public:
  FindAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, FindAvgOrientationsInputValues* inputValues);
  ~FindAvgOrientations() noexcept;

  FindAvgOrientations(const FindAvgOrientations&) = delete;            // Copy Constructor Not Implemented
  FindAvgOrientations(FindAvgOrientations&&) = delete;                 // Move Constructor Not Implemented
  FindAvgOrientations& operator=(const FindAvgOrientations&) = delete; // Copy Assignment Not Implemented
  FindAvgOrientations& operator=(FindAvgOrientations&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

protected:
private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const FindAvgOrientationsInputValues* m_InputValues = nullptr;
};

} // namespace complex

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
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

} // namespace nx::core

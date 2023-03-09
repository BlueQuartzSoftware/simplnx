#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureNeighborCAxisMisalignmentsInputValues inputValues;

  inputValues.FindAvgMisals = filterArgs.value<bool>(k_FindAvgMisals_Key);
  inputValues.NeighborListArrayPath = filterArgs.value<DataPath>(k_NeighborListArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CAxisMisalignmentListArrayName = filterArgs.value<DataPath>(k_CAxisMisalignmentListArrayName_Key);
  inputValues.AvgCAxisMisalignmentsArrayName = filterArgs.value<DataPath>(k_AvgCAxisMisalignmentsArrayName_Key);

  return FindFeatureNeighborCAxisMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindFeatureNeighborCAxisMisalignmentsInputValues
{
  bool FindAvgMisals;
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisMisalignmentListArrayName;
  DataPath AvgCAxisMisalignmentsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindFeatureNeighborCAxisMisalignments
{
public:
  FindFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        FindFeatureNeighborCAxisMisalignmentsInputValues* inputValues);
  ~FindFeatureNeighborCAxisMisalignments() noexcept;

  FindFeatureNeighborCAxisMisalignments(const FindFeatureNeighborCAxisMisalignments&) = delete;
  FindFeatureNeighborCAxisMisalignments(FindFeatureNeighborCAxisMisalignments&&) noexcept = delete;
  FindFeatureNeighborCAxisMisalignments& operator=(const FindFeatureNeighborCAxisMisalignments&) = delete;
  FindFeatureNeighborCAxisMisalignments& operator=(FindFeatureNeighborCAxisMisalignments&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureNeighborCAxisMisalignmentsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

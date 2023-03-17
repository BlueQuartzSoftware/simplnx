#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindMisorientationsInputValues
{
  bool FindAvgMisors;
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MisorientationListArrayName;
  DataPath AvgMisorientationsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindMisorientations
{
public:
  FindMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindMisorientationsInputValues* inputValues);
  ~FindMisorientations() noexcept;

  FindMisorientations(const FindMisorientations&) = delete;
  FindMisorientations(FindMisorientations&&) noexcept = delete;
  FindMisorientations& operator=(const FindMisorientations&) = delete;
  FindMisorientations& operator=(FindMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

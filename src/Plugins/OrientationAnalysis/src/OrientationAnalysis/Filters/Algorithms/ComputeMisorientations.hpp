#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeMisorientationsInputValues
{
  bool ComputeAvgMisors;
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MisorientationListArrayName;
  DataPath AvgMisorientationsArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeMisorientations
{
public:
  ComputeMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMisorientationsInputValues* inputValues);
  ~ComputeMisorientations() noexcept;

  ComputeMisorientations(const ComputeMisorientations&) = delete;
  ComputeMisorientations(ComputeMisorientations&&) noexcept = delete;
  ComputeMisorientations& operator=(const ComputeMisorientations&) = delete;
  ComputeMisorientations& operator=(ComputeMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborMisorientationsInputValues
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
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborMisorientations
{
public:
  ComputeFeatureNeighborMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        ComputeFeatureNeighborMisorientationsInputValues* inputValues);
  ~ComputeFeatureNeighborMisorientations() noexcept;

  ComputeFeatureNeighborMisorientations(const ComputeFeatureNeighborMisorientations&) = delete;
  ComputeFeatureNeighborMisorientations(ComputeFeatureNeighborMisorientations&&) noexcept = delete;
  ComputeFeatureNeighborMisorientations& operator=(const ComputeFeatureNeighborMisorientations&) = delete;
  ComputeFeatureNeighborMisorientations& operator=(ComputeFeatureNeighborMisorientations&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

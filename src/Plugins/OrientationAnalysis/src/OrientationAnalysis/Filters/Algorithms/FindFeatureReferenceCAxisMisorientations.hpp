#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceCAxisMisorientationsInputValues
{
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath AvgCAxesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureAvgCAxisMisorientationsArrayName;
  DataPath FeatureStdevCAxisMisorientationsArrayName;
  DataPath FeatureReferenceCAxisMisorientationsArrayName;
};

/**
 * @class FindFeatureReferenceCAxisMisorientations
 * @brief This filter calculates the misorientation angle between the C-axis of each Cell within a Feature and the average C-axis for that Feature and stores that value for each Cell.
 */

class ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceCAxisMisorientations
{
public:
  FindFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                           FindFeatureReferenceCAxisMisorientationsInputValues* inputValues);
  ~FindFeatureReferenceCAxisMisorientations() noexcept;

  FindFeatureReferenceCAxisMisorientations(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;
  FindFeatureReferenceCAxisMisorientations& operator=(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations& operator=(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureReferenceCAxisMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

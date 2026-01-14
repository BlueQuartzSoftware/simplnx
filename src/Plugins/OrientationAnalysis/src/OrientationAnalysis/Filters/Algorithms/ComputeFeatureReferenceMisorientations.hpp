#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientationsInputValues
{
  ChoicesParameter::ValueType ReferenceOrientation;
  DataPath FeatureAttributeMatrixPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath GBEuclideanDistancesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureReferenceMisorientationsArrayName;
  DataPath FeatureAvgMisorientationsArrayName;
  DataPath FeatureEuclideanCentersPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientations
{
public:
  ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         ComputeFeatureReferenceMisorientationsInputValues* inputValues);
  ~ComputeFeatureReferenceMisorientations() noexcept;

  ComputeFeatureReferenceMisorientations(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations(ComputeFeatureReferenceMisorientations&&) noexcept = delete;
  ComputeFeatureReferenceMisorientations& operator=(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations& operator=(ComputeFeatureReferenceMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureReferenceMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureFaceMisorientationInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  std::string SurfaceMeshFaceMisorientationColorsArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureFaceMisorientation
{
public:
  ComputeFeatureFaceMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    ComputeFeatureFaceMisorientationInputValues* inputValues);
  ~ComputeFeatureFaceMisorientation() noexcept;

  ComputeFeatureFaceMisorientation(const ComputeFeatureFaceMisorientation&) = delete;
  ComputeFeatureFaceMisorientation(ComputeFeatureFaceMisorientation&&) noexcept = delete;
  ComputeFeatureFaceMisorientation& operator=(const ComputeFeatureFaceMisorientation&) = delete;
  ComputeFeatureFaceMisorientation& operator=(ComputeFeatureFaceMisorientation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureFaceMisorientationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

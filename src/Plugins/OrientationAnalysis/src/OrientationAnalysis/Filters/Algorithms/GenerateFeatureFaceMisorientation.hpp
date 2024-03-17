#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT GenerateFeatureFaceMisorientationInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  std::string SurfaceMeshFaceMisorientationColorsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateFeatureFaceMisorientation
{
public:
  GenerateFeatureFaceMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    GenerateFeatureFaceMisorientationInputValues* inputValues);
  ~GenerateFeatureFaceMisorientation() noexcept;

  GenerateFeatureFaceMisorientation(const GenerateFeatureFaceMisorientation&) = delete;
  GenerateFeatureFaceMisorientation(GenerateFeatureFaceMisorientation&&) noexcept = delete;
  GenerateFeatureFaceMisorientation& operator=(const GenerateFeatureFaceMisorientation&) = delete;
  GenerateFeatureFaceMisorientation& operator=(GenerateFeatureFaceMisorientation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateFeatureFaceMisorientationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

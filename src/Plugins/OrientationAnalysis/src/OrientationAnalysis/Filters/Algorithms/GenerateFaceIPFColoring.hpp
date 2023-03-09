#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT GenerateFaceIPFColoringInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  std::string SurfaceMeshFaceIPFColorsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateFaceIPFColoring
{
public:
  GenerateFaceIPFColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateFaceIPFColoringInputValues* inputValues);
  ~GenerateFaceIPFColoring() noexcept;

  GenerateFaceIPFColoring(const GenerateFaceIPFColoring&) = delete;
  GenerateFaceIPFColoring(GenerateFaceIPFColoring&&) noexcept = delete;
  GenerateFaceIPFColoring& operator=(const GenerateFaceIPFColoring&) = delete;
  GenerateFaceIPFColoring& operator=(GenerateFaceIPFColoring&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateFaceIPFColoringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex

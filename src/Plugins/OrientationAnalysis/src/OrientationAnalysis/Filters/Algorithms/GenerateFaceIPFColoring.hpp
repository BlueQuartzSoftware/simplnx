#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
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
 * @class
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

} // namespace nx::core

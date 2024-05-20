#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeFaceIPFColoringInputValues
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
class ORIENTATIONANALYSIS_EXPORT ComputeFaceIPFColoring
{
public:
  ComputeFaceIPFColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFaceIPFColoringInputValues* inputValues);
  ~ComputeFaceIPFColoring() noexcept;

  ComputeFaceIPFColoring(const ComputeFaceIPFColoring&) = delete;
  ComputeFaceIPFColoring(ComputeFaceIPFColoring&&) noexcept = delete;
  ComputeFaceIPFColoring& operator=(const ComputeFaceIPFColoring&) = delete;
  ComputeFaceIPFColoring& operator=(ComputeFaceIPFColoring&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFaceIPFColoringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

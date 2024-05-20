#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDMetricBasedInputValues
{
  int32 PhaseOfInterest;
  VectorFloat32Parameter::ValueType MisorientationRotation;
  ChoicesParameter::ValueType ChosenLimitDists;
  int32 NumSamplPts;
  bool ExcludeTripleLines;
  FileSystemPathParameter::ValueType DistOutputFile;
  FileSystemPathParameter::ValueType ErrOutputFile;
  bool SaveRelativeErr;
  DataPath NodeTypesArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath SurfaceMeshFeatureFaceLabelsArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath TriangleGeometryPath;
};

/**
 * @class ComputeGBCDMetricBased
 * @brief This filter computes a section through the five-dimensional grain boundary distribution for a fixed mis orientation.
 */

class ORIENTATIONANALYSIS_EXPORT ComputeGBCDMetricBased
{
public:
  ComputeGBCDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDMetricBasedInputValues* inputValues);
  ~ComputeGBCDMetricBased() noexcept;

  ComputeGBCDMetricBased(const ComputeGBCDMetricBased&) = delete;
  ComputeGBCDMetricBased(ComputeGBCDMetricBased&&) noexcept = delete;
  ComputeGBCDMetricBased& operator=(const ComputeGBCDMetricBased&) = delete;
  ComputeGBCDMetricBased& operator=(ComputeGBCDMetricBased&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  static inline constexpr int k_NumberResolutionChoices = 7;
  static inline constexpr float k_ResolutionChoices[k_NumberResolutionChoices][2] = {{3.0f, 7.0f}, {5.0f, 5.0f}, {5.0f, 7.0f}, {5.0f, 8.0f},
                                                                                     {6.0f, 7.0f}, {7.0f, 7.0f}, {8.0f, 8.0f}}; // { for misorient., for planes }

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDMetricBasedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

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

  /**
   * @brief Checks the Fixed Misorientation. preflightImpl() and operator()() both call it.
   * @param misorientationRotation The h, k and l axis components followed by the angle in degrees.
   * @return k_InvalidMisorientationAngleError when the angle is not in (0, 180] (NaN included), or
   * k_InvalidMisorientationAxisError when an axis component is NaN or infinite or the axis is [0, 0, 0].
   */
  static Result<> ValidateMisorientationRotation(const std::vector<float32>& misorientationRotation);

  /// Error code for a Fixed Misorientation angle outside (0, 180] degrees.
  static constexpr int32 k_InvalidMisorientationAngleError = -7230;
  /// Error code for a Fixed Misorientation axis that is not finite or has length 0.
  static constexpr int32 k_InvalidMisorientationAxisError = -7231;

  /**
   * @brief Computes the metric-based GBCD section.
   * @return An error for an invalid Fixed Misorientation, an invalid Laue index, a file operation, or bulk I/O.
   */
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

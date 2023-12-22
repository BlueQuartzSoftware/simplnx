#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindGBPDMetricBasedInputValues
{
  int32 PhaseOfInterest;
  float64 LimitDist;
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
 * @class FindGBPDMetricBased
 * @brief This filter computes the grain boundary plane distribution (GBPD).
 */

class ORIENTATIONANALYSIS_EXPORT FindGBPDMetricBased
{
public:
  FindGBPDMetricBased(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindGBPDMetricBasedInputValues* inputValues);
  ~FindGBPDMetricBased() noexcept;

  FindGBPDMetricBased(const FindGBPDMetricBased&) = delete;
  FindGBPDMetricBased(FindGBPDMetricBased&&) noexcept = delete;
  FindGBPDMetricBased& operator=(const FindGBPDMetricBased&) = delete;
  FindGBPDMetricBased& operator=(FindGBPDMetricBased&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  static void AppendSamplePtsFixedZenith(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 theta, float64 minPhi, float64 maxPhi, float64 step);
  static void AppendSamplePtsFixedAzimuth(std::vector<float64>& xVec, std::vector<float64>& yVec, std::vector<float64>& zVec, float64 phi, float64 minTheta, float64 maxTheta, float64 step);

private:
  DataStructure& m_DataStructure;
  const FindGBPDMetricBasedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeTriangleGeomShapesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath CentroidsArrayPath;
  DataPath VolumesArrayPath;
  DataPath Omega3sArrayPath;
  DataPath AxisLengthsArrayPath;
  DataPath AxisEulerAnglesArrayPath;
  DataPath AspectRatiosArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeTriangleGeomShapes
{
public:
  ComputeTriangleGeomShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTriangleGeomShapesInputValues* inputValues);
  ~ComputeTriangleGeomShapes() noexcept;

  ComputeTriangleGeomShapes(const ComputeTriangleGeomShapes&) = delete;
  ComputeTriangleGeomShapes(ComputeTriangleGeomShapes&&) noexcept = delete;
  ComputeTriangleGeomShapes& operator=(const ComputeTriangleGeomShapes&) = delete;
  ComputeTriangleGeomShapes& operator=(ComputeTriangleGeomShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTriangleGeomShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<double> m_FeatureMoments;
  std::vector<double> m_FeatureEigenVals;
  void findMoments();
  void findAxes();
  void findAxisEulers();
};

} // namespace nx::core

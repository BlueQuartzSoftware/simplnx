#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindTriangleGeomShapesInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindTriangleGeomShapes
{
public:
  FindTriangleGeomShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomShapesInputValues* inputValues);
  ~FindTriangleGeomShapes() noexcept;

  FindTriangleGeomShapes(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes(FindTriangleGeomShapes&&) noexcept = delete;
  FindTriangleGeomShapes& operator=(const FindTriangleGeomShapes&) = delete;
  FindTriangleGeomShapes& operator=(FindTriangleGeomShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<double> m_FeatureMoments;
  std::vector<double> m_FeatureEigenVals;
  void findMoments();
  void findAxes();
  void findAxisEulers();
};

} // namespace nx::core

#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeShapesTriangleGeomInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath CentroidsArrayPath;
  DataPath Omega3sArrayPath;
  DataPath AxisLengthsArrayPath;
  DataPath AxisEulerAnglesArrayPath;
  DataPath AspectRatiosArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeShapesTriangleGeom
{
public:
  ComputeShapesTriangleGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeShapesTriangleGeomInputValues* inputValues);
  ~ComputeShapesTriangleGeom() noexcept;

  ComputeShapesTriangleGeom(const ComputeShapesTriangleGeom&) = delete;
  ComputeShapesTriangleGeom(ComputeShapesTriangleGeom&&) noexcept = delete;
  ComputeShapesTriangleGeom& operator=(const ComputeShapesTriangleGeom&) = delete;
  ComputeShapesTriangleGeom& operator=(ComputeShapesTriangleGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeShapesTriangleGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

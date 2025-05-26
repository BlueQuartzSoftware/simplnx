#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ShapeResultValues
{
  int32 featureId;
  float32 omega3;
  std::array<float32, 3> axisEulerAngles;
  std::array<float32, 3> axisLengths;
  std::array<float32, 2> aspectRatios;
};

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
  DataPath EulerCharacteristicPath;
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

  void updateResults(const std::vector<ShapeResultValues>& results);

private:
  DataStructure& m_DataStructure;
  const ComputeShapesTriangleGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_NumFeatures = 0;
  usize m_NumFeatureInc = 0;
  usize m_FeatureUpdateCount = 0;
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();

  Float32Array* m_Omega3s = nullptr;
  Float32Array* m_AxisEulerAngles = nullptr;
  Float32Array* m_AxisLengths = nullptr;
  Float32Array* m_AspectRatios = nullptr;
};
} // namespace nx::core

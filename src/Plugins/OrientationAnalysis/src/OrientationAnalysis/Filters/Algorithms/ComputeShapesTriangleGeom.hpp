#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "simplnx/DataStructure/DataArray.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <Eigen/Core>

#include <array>
#include <atomic>
#include <mutex>
#include <vector>

namespace nx::core
{

/**
 * @namespace ComputeShapesTriangleGeomDetail
 * @brief Internal mesh-ray measurements shared with direct contract tests.
 */
namespace ComputeShapesTriangleGeomDetail
{
/**
 * @struct AxialLengths
 * @brief Ordered principal-axis half-spans in mesh coordinate units.
 */
struct AxialLengths
{
  float64 xLength = 0.0;
  float64 yLength = 0.0;
  float64 zLength = 0.0;
};

/**
 * @brief Computes half-spans in the ordered principal-axis frame.
 * @param orientationMatrix Unit principal axes in rows.
 * @param featureFaces Triangle indices belonging to the feature.
 * @param triStore Triangle vertex indices.
 * @param vertexStore Vertex coordinates.
 * @param centroidsStore Feature centroids.
 * @param featureId Feature centroid tuple index.
 * @param shouldCancel Cancellation flag.
 * @return Half the sum of farthest hits in both directions for each row.
 * A missing direction contributes zero. Cancellation returns partial half-spans.
 * The existing MTIntersection ray-parameter tolerance applies to every hit.
 */
ORIENTATIONANALYSIS_EXPORT AxialLengths FindIntersections(const Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>& orientationMatrix, const std::vector<usize>& featureFaces,
                                                          const AbstractDataStore<IGeometry::MeshIndexType>& triStore, const AbstractDataStore<float32>& vertexStore,
                                                          const AbstractDataStore<float32>& centroidsStore, IGeometry::MeshIndexType featureId, const std::atomic_bool& shouldCancel);
} // namespace ComputeShapesTriangleGeomDetail

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
  ThrottledMessageHandler m_Throttle;

  Float32Array* m_Omega3s = nullptr;
  Float32Array* m_AxisEulerAngles = nullptr;
  Float32Array* m_AxisLengths = nullptr;
  Float32Array* m_AspectRatios = nullptr;
};
} // namespace nx::core

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
namespace compute_shapes_triangle_geom_detail
{
/**
 * @struct AxialLengths
 * @brief Ordered principal-axis half-spans in mesh coordinate units.
 */
struct AxialLengths
{
  AxialLengths() = default;
  AxialLengths(float64 x, float64 y, float64 z)
  : x_length(x)
  , y_length(y)
  , z_length(z)
  {
  }
  ~AxialLengths() noexcept = default;

  AxialLengths(const AxialLengths&) = default;
  AxialLengths(AxialLengths&&) noexcept = default;
  AxialLengths& operator=(const AxialLengths&) = default;
  AxialLengths& operator=(AxialLengths&&) noexcept = default;

  float64 x_length = 0.0;
  float64 y_length = 0.0;
  float64 z_length = 0.0;
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
  int32 feature_id;
  float32 omega3;
  std::array<float32, 3> axis_euler_angles;
  std::array<float32, 3> axis_lengths;
  std::array<float32, 2> aspect_ratios;
};

struct ORIENTATIONANALYSIS_EXPORT ComputeShapesTriangleGeomInputValues
{
  DataPath triangle_geometry_path;
  DataPath face_labels_array_path;
  DataPath feature_attribute_matrix_path;
  DataPath centroids_array_path;
  DataPath omega3s_array_path;
  DataPath axis_lengths_array_path;
  DataPath axis_euler_angles_array_path;
  DataPath aspect_ratios_array_path;
  DataPath euler_characteristic_path;
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

  mutable std::mutex m_ProgressMessageMutex;
  usize m_NumFeatures = 0;
  ThrottledMessageHandler m_Throttle;

  Float32ArrayType* m_Omega3s = nullptr;
  Float32ArrayType* m_AxisEulerAngles = nullptr;
  Float32ArrayType* m_AxisLengths = nullptr;
  Float32ArrayType* m_AspectRatios = nullptr;
};
} // namespace nx::core

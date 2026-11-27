#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeTriangleGeomCentroidsInputValues
{
  DataPath triangle_geometry_path;
  DataPath face_labels_array_path;
  DataPath feature_attribute_matrix_path;
  DataPath centroids_array_path;
  bool is_periodic = false;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeTriangleGeomCentroids
{
public:
  ComputeTriangleGeomCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTriangleGeomCentroidsInputValues* inputValues);
  ~ComputeTriangleGeomCentroids() noexcept = default;

  ComputeTriangleGeomCentroids(const ComputeTriangleGeomCentroids&) = delete;
  ComputeTriangleGeomCentroids(ComputeTriangleGeomCentroids&&) noexcept = delete;
  ComputeTriangleGeomCentroids& operator=(const ComputeTriangleGeomCentroids&) = delete;
  ComputeTriangleGeomCentroids& operator=(ComputeTriangleGeomCentroids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTriangleGeomCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT PointSampleEdgeGeometryInputValues
{
  float32 scan_vector_sampling_res{};
  DataPath scan_vector_geometry_path;
  DataPath sampled_vertex_geometry_path;
  bool calculate_cumulative_sample_distance{};
  std::string cumulative_sample_distance_array_name;
  std::string edge_ids_array_name;
  DataPath p_vertex_group_data_path;
  MultiArraySelectionParameter::ValueType p_selected_data_array_paths;
  MultiArraySelectionParameter::ValueType p_created_data_array_paths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT PointSampleEdgeGeometry
{
public:
  PointSampleEdgeGeometry(DataStructure& dataStructure, PointSampleEdgeGeometryInputValues* inputValues, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel);
  ~PointSampleEdgeGeometry() noexcept = default;

  PointSampleEdgeGeometry(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry(PointSampleEdgeGeometry&&) noexcept = delete;
  PointSampleEdgeGeometry& operator=(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry& operator=(PointSampleEdgeGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const PointSampleEdgeGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

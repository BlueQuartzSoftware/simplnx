#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct PartitionGeometryInputValues
 * @brief Stores partition-grid settings, paths, IDs, and mask options.
 */
struct SIMPLNXCORE_EXPORT PartitionGeometryInputValues
{
  ChoicesParameter::ValueType partitioning_mode{};
  int32 starting_feature_id{};
  int32 out_of_bounds_feature_id{};
  VectorInt32ParameterType::ValueType number_of_cells_per_axis;
  VectorFloat32ParameterType::ValueType partition_grid_origin;
  VectorFloat32ParameterType::ValueType cell_length;
  VectorFloat32ParameterType::ValueType min_grid_coord;
  VectorFloat32ParameterType::ValueType max_grid_coord;
  DataPath input_geom_cell_am_path;
  DataPath partition_grid_geom_path;
  std::string partition_grid_cell_am_name;
  std::string partition_grid_feature_i_ds_array_name;
  DataPath input_geometry_to_partition;
  std::string partition_ids_array_name;
  DataPath existing_partition_grid_path;
  bool use_vertex_mask{};
  DataPath vertex_mask_path;
  std::string feature_attr_matrix_name;
};

/**
 * @class PartitionGeometry
 * @brief Dispatches geometry partitioning from selected array storage.
 *
 * Dispatch targets include output IDs, created partition-grid IDs, node vertices,
 * and an optional vertex mask. RectGrid coordinate arrays are not dispatch targets.
 */
class SIMPLNXCORE_EXPORT PartitionGeometry
{
public:
  /**
   * @brief Defines the scalar store used for node coordinates.
   */
  using VertexStoreType = AbstractDataStore<IGeometry::SharedVertexListType::value_type>;

  /**
   * @brief Creates a geometry-partition dispatcher.
   * @param dataStructure Provides input geometry and partition outputs.
   * @param msgHandler Is retained for the dispatched interface.
   * @param shouldCancel Stops later initialization or partition work when true.
   * @param inputValues Specifies validated paths and partition settings. The caller
   * must keep this object alive for the dispatcher lifetime.
   */
  PartitionGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, PartitionGeometryInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~PartitionGeometry() noexcept = default;

  PartitionGeometry(const PartitionGeometry&) = delete;
  PartitionGeometry(PartitionGeometry&&) noexcept = delete;
  PartitionGeometry& operator=(const PartitionGeometry&) = delete;
  PartitionGeometry& operator=(PartitionGeometry&&) noexcept = delete;

  /**
   * @struct PSGeomInfo
   * @brief Stores dimensions and spatial metadata for a partition grid.
   */
  struct PSGeomInfo
  {
    USizeVec3Type geometry_dims;
    std::optional<FloatVec3Type> geometry_origin;
    std::optional<FloatVec3Type> geometry_spacing;
    IGeometry::LengthUnit geometry_units;
  };

  /**
   * @brief Selects direct or scanline partitioning from dispatch-target storage.
   * @return Error for an unknown geometry or bulk I/O, or success after cancellation.
   *
   * Cancellation or an I/O error can retain partial partition IDs. Created
   * partition-grid Feature IDs can also remain partial.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  PartitionGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

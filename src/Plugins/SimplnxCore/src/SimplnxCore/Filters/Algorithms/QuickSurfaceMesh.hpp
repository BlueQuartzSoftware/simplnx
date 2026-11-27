#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <string>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct QuickSurfaceMeshInputValues
 * @brief Stores parameters and created-object paths for QuickSurfaceMesh algorithms.
 */
struct SIMPLNXCORE_EXPORT QuickSurfaceMeshInputValues
{
  bool fix_problem_voxels{};      ///< When true, run iterative problem-voxel correction before meshing
  bool repair_triangle_winding{}; ///< When true, run winding repair on the output triangle mesh
  ChoicesParameter::ValueType bounding_box_skin_mode{};

  DataPath grid_geom_data_path;
  DataPath feature_ids_array_path;
  MultiArraySelectionParameter::ValueType selected_cell_data_array_paths;
  MultiArraySelectionParameter::ValueType selected_feature_data_array_paths;
  DataPath triangle_geometry_path;
  DataPath vertex_group_data_path;
  DataPath node_types_data_path;
  DataPath face_group_data_path;
  DataPath face_labels_data_path;
  MultiArraySelectionParameter::ValueType created_data_array_paths;
};

/**
 * @class QuickSurfaceMesh
 * @brief Selects the in-memory or scanline QuickSurfaceMesh implementation.
 *
 * Each cell face between different Feature IDs produces two triangles.
 * A boundary face uses -1 as its exterior Face Label.
 *
 * DispatchAlgorithm examines every selected source and created output array.
 * It selects QuickSurfaceMeshScanline when any target uses out-of-core (OOC) storage.
 * This selection avoids random chunk access and mesh-sized resident node state.
 * Test overrides can force either path for parity validation.
 *
 * @warning Concurrent dispatcher instances are not safe. Each implementation has
 * shared translation-unit random-number state for problem-voxel correction.
 *
 * @see QuickSurfaceMeshDirect, QuickSurfaceMeshScanline.
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMesh
{
public:
  /**
   * @brief Defines the storage interface for mesh vertex coordinates.
   */
  using VertexStoreType = AbstractDataStore<IGeometry::SharedVertexListType::value_type>;
  /**
   * @brief Defines the storage interface for mesh triangle connectivity.
   */
  using TriStoreType = AbstractDataStore<IGeometry::SharedTriListType::value_type>;
  /**
   * @brief Defines the integer type for mesh indices.
   */
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure DataStructure that must outlive the dispatcher.
   * @param inputValues Non-null inputs that must outlive the dispatcher.
   * @param shouldCancel Atomic cancellation flag that must outlive the dispatcher.
   * @param mesgHandler Message callback that must outlive the dispatcher.
   * @pre inputValues is not null.
   *
   * The dispatcher borrows all arguments.
   */
  QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

  /**
   * @brief Destroys the dispatcher.
   */
  ~QuickSurfaceMesh() noexcept = default;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  /**
   * @brief Runs the implementation selected from all source and output storage types.
   * @return Result from the selected implementation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core

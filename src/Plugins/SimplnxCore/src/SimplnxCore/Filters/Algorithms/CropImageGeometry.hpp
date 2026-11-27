#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct CropImageGeometryInputValues
 * @brief Collects crop settings, paths, and effective inclusive voxel bounds.
 */
struct SIMPLNXCORE_EXPORT CropImageGeometryInputValues
{
  GeometrySelectionParameter::ValueType input_image_geometry_path;
  DataGroupCreationParameter::ValueType output_image_geometry_path;
  ArraySelectionParameter::ValueType feature_ids_path;
  VectorUInt64ParameterType::ValueType min_voxel;
  VectorUInt64ParameterType::ValueType max_voxel;
  BoolParameter::ValueType clear_feature_attribute_matrix{};
  BoolParameter::ValueType renumber_features{};
  AttributeMatrixSelectionParameter::ValueType cell_feature_attribute_matrix_path;
  BoolParameter::ValueType remove_original_geometry{};
  BoolParameter::ValueType crop_x_dim{};
  BoolParameter::ValueType crop_y_dim{};
  BoolParameter::ValueType crop_z_dim{};

  // Preflight computes these inclusive voxel indices.
  uint64 x_min{};
  uint64 x_max{};
  uint64 y_min{};
  uint64 y_max{};
  uint64 z_min{};
  uint64 z_max{};
};

/**
 * @class CropImageGeometry
 * @brief Copies an inclusive voxel region to a smaller ImageGeom.
 *
 * Optionally renumbers Feature Ids and resizes the cleared Feature Attribute Matrix.
 *
 * Resident pairs copy selected rows directly.
 * Other pairs use buffers with a 1 MiB total cap for
 * each task.
 * Storage backends and caches can allocate more memory.
 */
class SIMPLNXCORE_EXPORT CropImageGeometry
{
public:
  /**
   * @brief Initializes image-geometry cropping.
   * @param dataStructure Contains source and destination geometry.
   * @param mesgHandler Receives per-array progress messages.
   * @param shouldCancel Signals cancellation between row or slab transfers.
   * @param inputValues Identifies paths, options, and effective bounds.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept = default;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  /**
   * @brief Copies cell arrays and optionally renumbers Feature Ids and resizes the cleared Feature Attribute Matrix.
   * @return Success, or a validation, transfer, or feature-processing error.
   *
   * Cell-array tasks can run concurrently across separate arrays. The shared
   * task result propagates the first transfer error.
   *
   * Cancellation returns success.
   * Each started task fills
   * its destination before transfers.
   * Completed transfers and prior structural changes remain.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CropImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core

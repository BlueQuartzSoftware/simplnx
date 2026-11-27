#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>
#include <vector>

namespace nx::core
{
/**
 * @struct ImageGeometryCropBounds
 * @brief Stores inclusive voxel bounds calculated during crop preflight.
 */
struct SIMPLNX_EXPORT ImageGeometryCropBounds
{
  uint64 x_min = 0;
  uint64 x_max = 0;
  uint64 y_min = 0;
  uint64 y_max = 0;
  uint64 z_min = 0;
  uint64 z_max = 0;
};

/**
 * @struct ImageGeometryCropOptions
 * @brief Stores typed inputs for shared Image Geometry crop planning.
 */
struct SIMPLNX_EXPORT ImageGeometryCropOptions
{
  DataPath input_image_geometry_path;
  DataPath output_image_geometry_path;
  DataPath feature_ids_path;
  DataPath cell_feature_attribute_matrix_path;
  std::vector<uint64> min_voxel = {0, 0, 0};
  std::vector<uint64> max_voxel = {0, 0, 0};
  std::vector<float64> min_coordinate = {0.0, 0.0, 0.0};
  std::vector<float64> max_coordinate = {0.0, 0.0, 0.0};
  // Recreate the Feature AM with no arrays; required for renumberFeatures.
  bool clear_feature_attribute_matrix = false;
  // Report unchanged Feature data only for callers that request this information.
  bool report_uncleared_feature_data = false;
  bool renumber_features = false;
  bool remove_original_geometry = false;
  bool use_physical_bounds = false;
  bool crop_x = true;
  bool crop_y = true;
  bool crop_z = true;
};

/**
 * @brief Plans Image Geometry crop actions without a plugin dependency.
 * @param dataStructure Contains the source Image Geometry and child objects.
 * @param options Contains typed crop settings.
 * @param bounds Receives inclusive voxel bounds for execution.
 * @return Crop actions, warnings, display values, or errors.
 */
SIMPLNX_EXPORT IFilter::PreflightResult PreflightImageGeometryCrop(const DataStructure& dataStructure, const ImageGeometryCropOptions& options, ImageGeometryCropBounds& bounds);
} // namespace nx::core

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
  uint64 xMin = 0;
  uint64 xMax = 0;
  uint64 yMin = 0;
  uint64 yMax = 0;
  uint64 zMin = 0;
  uint64 zMax = 0;
};

/**
 * @struct ImageGeometryCropOptions
 * @brief Stores typed inputs for shared Image Geometry crop planning.
 */
struct SIMPLNX_EXPORT ImageGeometryCropOptions
{
  DataPath inputImageGeometryPath;
  DataPath outputImageGeometryPath;
  DataPath featureIdsPath;
  DataPath cellFeatureAttributeMatrixPath;
  std::vector<uint64> minVoxel = {0, 0, 0};
  std::vector<uint64> maxVoxel = {0, 0, 0};
  std::vector<float64> minCoordinate = {0.0, 0.0, 0.0};
  std::vector<float64> maxCoordinate = {0.0, 0.0, 0.0};
  bool renumberFeatures = false;
  bool removeOriginalGeometry = false;
  bool usePhysicalBounds = false;
  bool cropX = true;
  bool cropY = true;
  bool cropZ = true;
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

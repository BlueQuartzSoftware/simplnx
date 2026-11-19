#pragma once

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <map>
#include <string>

namespace nx::core::DREAM3D
{
/** @brief Origin and spacing read from an image geometry before validation. */
struct ImageGeometryMetadata
{
  FloatVec3 origin;  /**< Stored origin in X, Y, Z order. */
  FloatVec3 spacing; /**< Stored spacing in X, Y, Z order. */
};

/**
 * @brief Stored image metadata or read errors, keyed by the geometry's imported identifier.
 *
 * Selective discovery records these results so callers can validate only the geometries they import.
 */
using DeferredImageMetadata = std::map<DataObject::IdType, Result<ImageGeometryMetadata>>;
} // namespace nx::core::DREAM3D

namespace nx::core::DREAM3D::detail
{
inline constexpr int32 k_DefaultImageGeometryValuesWarning = 502693;

inline std::string ImageGeometryPath(const ImageGeom& geometry)
{
  const auto paths = geometry.getDataPaths();
  return paths.empty() ? geometry.getName() : paths.front().toString();
}

/**
 * @brief Installs valid image metadata and optionally uses defaults for invalid stored components.
 * @param geometry Destination image geometry.
 * @param filePath Path to the input DREAM.3D file.
 * @param metadata Stored origin and spacing, or the error encountered reading them.
 * @param useDefaultGeometryValues Whether to use 0 for invalid origin components and 1 for invalid spacing components.
 * @return Success, with warning 502693 if any component uses a default. With defaults disabled, returns
 * -1556 for nonfinite or nonpositive spacing or -1557 for a nonfinite origin. Validation errors leave the geometry unchanged.
 */
[[nodiscard]] inline Result<> ApplyImportedImageMetadata(ImageGeom& geometry, const std::filesystem::path& filePath, const Result<ImageGeometryMetadata>& metadata, bool useDefaultGeometryValues)
{
  if(metadata.invalid())
  {
    return {nonstd::make_unexpected(metadata.errors()), metadata.warnings()};
  }
  auto [origin, spacing] = metadata.value();
  auto validation = ImageGeom::ValidateSpacing(spacing);
  if(validation.valid())
  {
    validation = ImageGeom::ValidateOrigin(origin);
  }

  std::string defaultedComponents;
  std::string geometryPath;
  if(validation.invalid())
  {
    geometryPath = ImageGeometryPath(geometry);
    if(!useDefaultGeometryValues)
    {
      auto& error = validation.errors().front();
      error.message = fmt::format("Cannot read image geometry '{}' from '{}'. {}", geometryPath, filePath.string(), error.message);
      return validation;
    }

    constexpr std::array<char, 3> k_Axes = {'X', 'Y', 'Z'};
    for(usize i = 0; i < k_Axes.size(); ++i)
    {
      if(!std::isfinite(spacing[i]) || !(spacing[i] > 0.0F))
      {
        defaultedComponents += fmt::format(" Spacing {} stored {}, using 1.", k_Axes[i], spacing[i]);
        spacing[i] = 1.0F;
      }
    }
    for(usize i = 0; i < k_Axes.size(); ++i)
    {
      if(!std::isfinite(origin[i]))
      {
        defaultedComponents += fmt::format(" Origin {} stored {}, using 0.", k_Axes[i], origin[i]);
        origin[i] = 0.0F;
      }
    }
  }

  auto result = geometry.setOrigin(origin);
  if(result.invalid())
  {
    return result;
  }
  result = geometry.setSpacing(spacing);
  if(result.valid() && !defaultedComponents.empty())
  {
    result.warnings().push_back({k_DefaultImageGeometryValuesWarning, fmt::format("File '{}' has invalid values for image geometry '{}'. Using default values instead:{} The file is not changed.",
                                                                                  filePath.string(), geometryPath, defaultedComponents)});
  }
  return result;
}
/**
 * @brief Records unvalidated metadata for selective import, or installs it immediately for a direct read.
 */
[[nodiscard]] inline Result<> ImportImageMetadata(ImageGeom& geometry, const std::filesystem::path& filePath, Result<ImageGeometryMetadata> metadata, DeferredImageMetadata* deferredMetadata)
{
  if(deferredMetadata != nullptr)
  {
    deferredMetadata->emplace(geometry.getId(), std::move(metadata));
    return {};
  }
  return ApplyImportedImageMetadata(geometry, filePath, metadata, false);
}
} // namespace nx::core::DREAM3D::detail

#pragma once

#include "simplnx/Common/Types.hpp"

namespace nx::core
{

/**
 * @brief Controls when origin/spacing overrides are applied relative to cropping operations.
 *
 * The integer values match the index order of the "Origin & Spacing Processing" ChoicesParameter
 * used by the image-reading filters, so a static_cast<> from the raw ChoicesParameter::ValueType
 * is valid.
 */
enum class OriginSpacingProcessing : uint64
{
  Preprocessed = 0,  ///< Overrides are applied before cropping.
  Postprocessed = 1, ///< Overrides are applied after cropping.
};

/**
 * @brief Per-slice flip operation applied when importing an image stack.
 *
 * The integer values match the index order of the "Flip Slice" ChoicesParameter used by the
 * image-stack-reading filters, so static_cast<> from the raw ChoicesParameter::ValueType is valid.
 */
enum class ImageFlipTransform : uint64
{
  None = 0,
  FlipAboutXAxis = 1,
  FlipAboutYAxis = 2,
};

/**
 * @brief Backend selected by the unified image reader based on the input file extension.
 *
 * Raster covers the raw-raster formats read through the IImageIO abstraction (png/jpg/bmp/tif,
 * including multi-page TIFF Z-stacks). Nrrd covers .nrrd/.nhdr volumes read through the NRRD
 * header parser + streaming reader. (.mha is handled by a separate transform-aware filter.)
 */
enum class ReadImageBackend : uint8
{
  Raster = 0,
  Nrrd = 1,
};

} // namespace nx::core

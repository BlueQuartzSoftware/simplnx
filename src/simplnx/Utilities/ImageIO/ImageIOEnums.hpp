#pragma once

#include "simplnx/Common/Types.hpp"

namespace nx::core
{

/**
 * @brief Controls when origin/spacing overrides are applied relative to cropping operations.
 *
 * The integer values match the index order of the "Origin & Spacing Processing" ChoicesParameter
 * used by the image-reading filters, so reinterpret_cast<> or static_cast<> from the raw
 * ChoicesParameter::ValueType is valid.
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

} // namespace nx::core

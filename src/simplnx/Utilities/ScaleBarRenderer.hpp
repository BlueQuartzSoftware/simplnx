#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

#include <string>
#include <vector>

namespace nx::core::ScaleBarRenderer
{
/**
 * @brief Picks the physical length of a scale bar for an image: the largest
 * "nice" value (1, 2 or 5 times a power of ten) that is no more than 25% of
 * the physical width of the image.
 * @param imageWidthPixels Width of the image in pixels
 * @param unitsPerPixel Physical size of one pixel along the horizontal axis, in geometry units
 * @return The bar length in geometry units, or 0.0 if the inputs are degenerate
 */
SIMPLNX_EXPORT float64 ComputeNiceBarLength(usize imageWidthPixels, float64 unitsPerPixel);

/**
 * @brief Formats a physical length as a human readable label such as "100 µm".
 * Metric units are rescaled to the engineering prefix that gives a mantissa in
 * [1, 1000). Non-metric units (Inch, Foot, ...) are labeled as-is. Unspecified
 * and Unknown units omit the unit suffix.
 * @param lengthInUnits The length expressed in the geometry's units
 * @param unit The geometry's length unit
 * @return The formatted label
 */
SIMPLNX_EXPORT std::string FormatLengthLabel(float64 lengthInUnits, IGeometry::LengthUnit unit);

/**
 * @brief Computes the height in pixels of the scale-bar band appended below an
 * image: 8% of the image height, clamped to a minimum of 24 pixels.
 * @param imageHeightPixels Height of the image in pixels
 * @return The band height in pixels
 */
SIMPLNX_EXPORT usize ComputeBandHeight(usize imageHeightPixels);

/**
 * @brief Renders the scale-bar band as a packed 3-component RGB image of size
 * imageWidthPixels x ComputeBandHeight(imageHeightPixels): a white background
 * with a centered black bar near the bottom and the length label centered
 * above it. The caller appends these rows below the image rows.
 * @param imageWidthPixels Width of the image (and the band) in pixels
 * @param imageHeightPixels Height of the image the band will be appended to
 * @param unitsPerPixel Physical size of one pixel along the horizontal axis, in geometry units
 * @param unit The geometry's length unit
 * @return RGB pixel buffer of size imageWidthPixels * ComputeBandHeight(imageHeightPixels) * 3
 */
SIMPLNX_EXPORT std::vector<uint8> RenderScaleBarBandRgb(usize imageWidthPixels, usize imageHeightPixels, float64 unitsPerPixel, IGeometry::LengthUnit unit);
} // namespace nx::core::ScaleBarRenderer

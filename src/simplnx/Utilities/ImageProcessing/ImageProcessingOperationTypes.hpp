#pragma once

namespace nx::core::ImageProcessing
{
/**
 * @brief The grayscale morphology operation to apply through a flat structuring element.
 *
 * Dilate folds neighbors by max; Erode folds by min. The boundary is handled by SKIPPING
 * out-of-bounds structuring-element neighbors, which is bit-identical to ITK 5.4.4's grayscale
 * morphology substituting a constant type extremum for out-of-range voxels (Dilate ->
 * std::numeric_limits<T>::lowest(), Erode -> std::numeric_limits<T>::max()): because the fold is
 * seeded with that same extremum, an out-of-range neighbor filled with the extremum can never win
 * the max/min and so is equivalent to dropping it.
 */
enum class MorphOp
{
  Dilate,
  Erode
};

/**
 * @brief The object-morphology operation to apply through a flat structuring element.
 *
 * Object morphology is a boundary-SCATTER (NOT the min/max SE-gather of grayscale/binary morphology): only
 * the BOUNDARY voxels of the object (voxels equal to the object value that touch at least one non-object
 * voxel in the fixed radius-1 full box) paint the structuring element around themselves into a SEPARATE
 * output that starts as a copy of the input. Dilate paints the object value; Erode paints the background
 * value. This reproduces ITK 5.4.4's itkObjectMorphologyImageFilter family exactly.
 */
enum class ObjectMorphOp
{
  Dilate,
  Erode
};

/**
 * @brief The morphological reconstruction direction. Reconstruction-by-dilation grows the marker toward maxima,
 *        bounded above by the mask (requires marker <= mask); reconstruction-by-erosion shrinks toward minima,
 *        bounded below by the mask (requires marker >= mask).
 */
enum class ReconstructOp
{
  Dilation,
  Erosion
};

/**
 * @brief Regional-extrema direction. Maxima keeps regional maxima (flat zones with no strictly-greater neighbor);
 *        Minima keeps regional minima. The non-extrema are set to the marker value (type lowest for maxima, type
 *        max for minima).
 */
enum class RegionalExtremaOp
{
  Maxima,
  Minima
};
} // namespace nx::core::ImageProcessing

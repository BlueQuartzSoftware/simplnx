# Robust Automatic Threshold

## Group (Subgroup)

Threshold

## Description

This filter is *automatic* because it computes the threshold value itself, and *robust* because it derives that value from where the data changes most sharply rather than from a fixed cutoff. The user does not have to choose a threshold value manually; the filter selects one that tends to separate the input array along its sharpest boundaries. It produces a boolean **Mask** array that is *false* where the input array is less than the computed threshold and *true* otherwise.

The threshold is computed as a gradient-magnitude-weighted average of the input array. Each value of the input array is weighted by its corresponding gradient magnitude, so **Cells** that sit on strong boundaries (high gradient magnitude) contribute most to the chosen threshold, while flat interior regions (low gradient magnitude) contribute little. The result is a single threshold value `T` that generally partitions the input array where its gradient is highest.

The *gradient magnitude* measures how quickly the input field changes from one location to the next: it is large at edges or boundaries and near zero in smooth regions. It is obtained by computing the *2-norm* (the square root of the sum of the squares of the per-axis derivative components) of the gradient vector at each **Cell**, which collapses the multi-component gradient into a single non-negative scalar value per **Cell**. The required gradient magnitude array can be produced with the [ITK Gradient Magnitude Image Filter](../ITKImageProcessing/ITKGradientMagnitudeImageFilter.md).

### Required Input Sources

- **Gradient Magnitude Data** -- a single-component 32-bit float array giving the gradient magnitude of the input array, produced by [ITK Gradient Magnitude Image Filter](../ITKImageProcessing/ITKGradientMagnitudeImageFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

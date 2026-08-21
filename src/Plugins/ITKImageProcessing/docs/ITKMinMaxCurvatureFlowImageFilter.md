# ITK Min Max Curvature Flow Image Filter

Edge-preserving denoising like Curvature Flow, but with a switch that targets a chosen noise scale and better preserves real features.

## Group (Subgroup)

ITKCurvatureFlow (CurvatureFlow)

## Description

This filter is a variant of [ITK Curvature Flow Image Filter](ITKCurvatureFlowImageFilter.md). Plain curvature flow smooths every intensity contour according to its curvature; the **min/max** variant adds a switch that turns smoothing on or off at each point depending on the local image content, so that smoothing is applied where it removes noise but suppressed where it would erode a genuine feature.

The switch is keyed to the **stencil radius**: the filter looks at the average intensity in a neighborhood of that radius around each point and uses it to decide whether the point sits on noise (smooth it) or on a real structure (leave it). Choosing the stencil radius therefore selects the *scale* of the noise to remove — a small radius targets fine, single-pixel noise; a larger radius targets coarser texture.

The output pixels are of a floating-point type.

### Parameter Guidance

- **Number Of Iterations** — how many time steps to apply. More iterations smooth more strongly.
- **Time Step** — the size of each step (dimensionless). Keep it small for numerical stability (around **0.125 for 2D** and **0.0625 for 3D**; default 0.05) and reduce if the result looks unstable.
- **Stencil Radius** — the neighborhood radius, **in pixels**, used by the min/max switch; it sets the scale of noise that is removed. Small values (the default is *2*) target fine noise; larger values target coarser features. (Stored as a signed integer, but only non-negative values are meaningful.)

### Required Input Sources

Operates on any scalar (floating-point) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

## Reference

"Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.

## See Also

- [ITK MinMaxCurvatureFlowImageFilter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1MinMaxCurvatureFlowImageFilter.html)
- [ITK Curvature Flow Image Filter](ITKCurvatureFlowImageFilter.md)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

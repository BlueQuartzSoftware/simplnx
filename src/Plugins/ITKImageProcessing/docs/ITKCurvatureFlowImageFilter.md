# ITK Curvature Flow Image Filter

Edge-preserving denoising that smooths an image by moving its intensity contours according to their curvature.

## Group (Subgroup)

ITKCurvatureFlow (CurvatureFlow)

## Description

**Curvature flow** is an edge-preserving smoothing method. The grayscale image is treated as a stack of iso-intensity contours (lines of constant brightness, like the contour lines on a topographic map). Each contour is then moved at a speed proportional to its local curvature: tightly curved, wiggly parts of a contour (typically noise) move quickly and flatten out, while straight, low-curvature parts (typically real boundaries) barely move. The result is that small noise is smoothed away while sharp edges are preserved.

The evolution is applied over a number of small time steps. Note that applying too many iterations eventually shrinks every contour to nothing and erases all detail, so the iteration count should be tuned to the image.

A faster, scale-selective variant is available in [ITK Min Max Curvature Flow Image Filter](ITKMinMaxCurvatureFlowImageFilter.md). The output pixels are of a floating-point type.

### Parameter Guidance

- **Number Of Iterations** — how many time steps to apply. More iterations smooth more strongly; too many erase real features.
- **Time Step** — the size of each step (dimensionless). It must be small enough for numerical stability (each contour should move less than one grid cell per step). As a practical starting point use values around **0.125 for 2D** and **0.0625 for 3D** images and reduce if the result looks unstable. (Default 0.05.)

### Required Input Sources

Operates on any scalar (floating-point) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

## Reference

"Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.

## See Also

- [ITK CurvatureFlowImageFilter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1CurvatureFlowImageFilter.html)
- [ITK Min Max Curvature Flow Image Filter](ITKMinMaxCurvatureFlowImageFilter.md)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

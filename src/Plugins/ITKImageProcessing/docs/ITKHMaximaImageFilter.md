# ITK H Maxima Image Filter

Flattens shallow bright peaks by suppressing local maxima whose height is less than *h*.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **H-Maxima** transform removes insignificant bright peaks from a grayscale image. Any local maximum that rises **less than *h* above its local background** is suppressed — flattened down to an estimate of that background — while larger, genuine peaks are preserved. This smooths over the "high" parts of the noise without blurring real region boundaries or large intensity changes.

Use this filter to clean up speckle and spurious bright spots before segmentation, or as a preconditioning step for peak/maxima detection. Subtracting this filter's output from the original image yields the prominent peaks themselves (the operation performed by the [H Convex](ITKHConvexImageFilter.md) filter); the complementary operation for dark features is the [H Minima](ITKHMinimaImageFilter.md) filter.

### Parameter Guidance

- **Height** — the height *h*, in the **input image's intensity units**. Maxima that rise less than *h* above their local background are suppressed; peaks taller than *h* survive. Larger values flatten more peaks. Default *2.0*.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

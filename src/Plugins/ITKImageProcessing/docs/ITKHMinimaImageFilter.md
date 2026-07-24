# ITK H Minima Image Filter

Fills shallow dark valleys by suppressing local minima whose depth is less than *h*.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **H-Minima** transform removes insignificant dark dips from a grayscale image. Any local minimum that lies **less than *h* below its local background** is filled in — raised to an estimate of that background — while deeper, genuine valleys are preserved. This smooths over the "low" parts of the noise without blurring real region boundaries or large intensity changes.

Use this filter to clean up dark speckle and spurious low spots before segmentation, or as a preconditioning step for minima detection (for example to control over-segmentation in a watershed). It is the dark-feature counterpart of the [H Maxima](ITKHMaximaImageFilter.md) filter; the matching operation that *extracts* prominent bright peaks is the [H Convex](ITKHConvexImageFilter.md) filter.

### Parameter Guidance

- **Height** — the depth *h*, in the **input image's intensity units**. Minima that lie less than *h* below their local background are filled; valleys deeper than *h* survive. Larger values fill more dips. Default *2.0*.
- **Fully Connected** — controls pixel connectivity. When off (default), neighbors share a face; when on, neighbors also include edge- and corner-touching pixels. Turn it on for thin, one-pixel-wide features.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

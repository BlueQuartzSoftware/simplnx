# ITK Signed Maurer Distance Map Image Filter

Computes an exact Euclidean signed distance map of a binary image in linear time.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

A **distance map** replaces every pixel with its distance to the nearest object boundary. This filter computes the **exact** Euclidean distance (not an approximation) using the Maurer algorithm, which runs in linear time for images of any dimension.

It produces a **signed** distance map: pixels **inside** an object are given **negative** distances and pixels **outside** are given **positive** distances. By default inside is negative; enable *Inside Is Positive* to flip the convention. Unlike the Danielsson filters, this filter does not produce a Voronoi map.

Use this when you need an accurate signed Euclidean distance field — for example as a level-set input or for precise distance-from-boundary measurements.

> Important — the default output is the **squared** distance, not the distance itself. *Squared Distance* is enabled by default so the filter can stay in fast integer arithmetic. To get true (un-squared) Euclidean distance, turn *Squared Distance* off.

> Note: for true Euclidean distances, or when *Use Image Spacing* is enabled, choose a floating-point output array type. An integer output can only represent the squared, pixel-unit distances correctly.

### Parameter Guidance

- **Inside Is Positive** — by default, pixels inside an object are negative and pixels outside are positive. Enable this to reverse the convention.
- **Squared Distance** — when enabled (the default), the output holds the distance **squared**. Disable it to get the actual (linear) distance.
- **Use Image Spacing** — when enabled, distances are measured in the image's **physical units** (using pixel spacing). When disabled, distances are measured in **pixels**.
- **Background Value** — the input intensity that marks the background. Object pixels are everything else. Normally *0* (the default).

### Required Input Sources

Requires a binary image where the *Background Value* marks background and all other pixels mark object. This is typically a segmentation mask produced upstream.

## See Also

- [ITK Danielsson Distance Map Image Filter](ITKDanielssonDistanceMapImageFilter.md)
- [ITK Signed Maurer Distance Map Image Filter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1SignedMaurerDistanceMapImageFilter.html)

Reference: C. R. Maurer, Jr., R. Qi, and V. Raghavan, "A Linear Time Algorithm for Computing Exact Euclidean Distance Transforms of Binary Images in Arbitrary Dimensions," IEEE Transactions on Pattern Analysis and Machine Intelligence, 25(2): 265-270, 2003.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

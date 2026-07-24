# ITK Approximate Signed Distance Map Image Filter

Computes a fast, approximate signed distance map from the object boundaries in a binary image.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

A **distance map** replaces every pixel with its distance to the nearest object boundary. This filter produces a **signed** distance map: pixels inside an object are given **negative** distances, and pixels outside an object are given **positive** distances. The magnitude of each value is the (approximate) distance, in **pixels**, from that pixel to the closest object boundary.

The distances are **Chamfer distances**, a fast approximation to true Euclidean distance. This filter trades some accuracy for speed. If you need a closer approximation to Euclidean distance, use the [Danielsson](ITKDanielssonDistanceMapImageFilter.md) or [Signed Maurer](ITKSignedMaurerDistanceMapImageFilter.md) distance maps instead.

Use a signed distance map when you need a smooth field that grows away from object boundaries in both directions — for example, as a speed/level-set input, for shape morphing, or to find points a fixed distance from a feature edge.

> Note: distances are computed and stored as floating-point values. Choose a floating-point output array type; an integer output cannot represent fractional or negative distances correctly.

### Parameter Guidance

The binary input is interpreted using two intensity values:

- **Inside Value** — the pixel intensity that marks the **object** (foreground). Default *1*.
- **Outside Value** — the pixel intensity that marks the **background**. Default *0*.

For example, in a typical mask where objects are labeled *1* and background is *0*, set *Inside Value = 1* and *Outside Value = 0*. If your mask labels objects as *255* on a *0* background, set *Inside Value = 255* and *Outside Value = 0*.

The filter runs slightly faster when *Inside Value* is less than *Outside Value*; otherwise an extra pass over the image is required.

### Required Input Sources

Requires a binary (two-value) image where one intensity marks objects and the other marks background. This is typically a segmentation mask produced by a thresholding or segmentation filter upstream.

## See Also

- [ITK Danielsson Distance Map Image Filter](ITKDanielssonDistanceMapImageFilter.md)
- [ITK Signed Danielsson Distance Map Image Filter](ITKSignedDanielssonDistanceMapImageFilter.md)
- [ITK Signed Maurer Distance Map Image Filter](ITKSignedMaurerDistanceMapImageFilter.md)
- [ITK ApproximateSignedDistanceMapImageFilter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1ApproximateSignedDistanceMapImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

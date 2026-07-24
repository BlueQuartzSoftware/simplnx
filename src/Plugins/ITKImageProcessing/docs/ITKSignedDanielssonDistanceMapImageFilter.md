# ITK Signed Danielsson Distance Map Image Filter

Computes a near-Euclidean signed distance map of a binary image, with inside distances negative.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

A **distance map** replaces every pixel with its distance to the nearest object boundary. This is the **signed** variant of the Danielsson distance map: pixels **inside** an object are given **negative** distances and pixels **outside** are given **positive** distances. The distance is measured from the boundary of the object (the ON pixels). By default, inside is negative; enable *Inside Is Positive* to flip the convention.

The input is treated as binary: pixels with value 0 are background and all nonzero pixels are object. Distances are a close approximation to true Euclidean distance.

Like the unsigned Danielsson filter, it can also report:

- **Voronoi partition** — each background pixel labeled with the closest object, dividing the image into nearest-influence regions.
- **Offset / vector map** — for each pixel, the per-axis displacement (in pixels) pointing to its closest object point.

Use a signed distance map when you need a smooth field that changes sign at object boundaries — for example as a level-set or shape-morphing input.

### Parameter Guidance

- **Inside Is Positive** — by default, pixels inside an object are negative and pixels outside are positive. Enable this to reverse that convention (inside positive, outside negative).
- **Squared Distance** — when enabled, the output holds the distance **squared** instead of the distance. This avoids a square-root step but the values are no longer in linear distance units. Leave off for true distances.
- **Use Image Spacing** — when enabled, distances are measured in the image's **physical units** (using pixel spacing). When disabled, distances are measured in **pixels**.

### Required Input Sources

Requires a binary image: zero marks background and any nonzero value marks object. This is typically a segmentation mask produced upstream.

## See Also

- [ITK Danielsson Distance Map Image Filter](ITKDanielssonDistanceMapImageFilter.md)
- [ITK Signed Danielsson Distance Map Image Filter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1SignedDanielssonDistanceMapImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

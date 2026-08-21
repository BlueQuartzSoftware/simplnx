# ITK Danielsson Distance Map Image Filter

Computes a near-Euclidean distance map of an image, with optional Voronoi partition and nearest-object offset map.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

A **distance map** replaces every pixel with its distance to the nearest object. This filter computes a close approximation to the true Euclidean distance using the Danielsson algorithm, which is fast and accurate to about one pixel.

The input is treated as a set of labeled objects (any nonzero pixel belongs to an object). The filter can produce three related results:

- **Distance map** — each pixel holds the (approximate Euclidean) distance to the nearest object pixel. This is the primary output.
- **Voronoi partition** — a labeling in which every background pixel is assigned the label of the closest object, dividing the image into regions of nearest influence (like territory maps around each object).
- **Offset / vector map** — for each pixel, the displacement (in pixels along each axis) that points from the pixel to its closest object point. This records the direction to the nearest object, not just the distance.

Use a distance map when you need to know how far each location is from a feature — for example, to find region centers (local distance maxima), to seed a watershed, or to grow regions outward from objects.

### Parameter Guidance

- **Input Is Binary** — when enabled, each nonzero input pixel is given a unique code so the Voronoi partition can separate individual objects. Leave it off if your input already carries distinct object labels, or if you do not need per-object Voronoi regions.
- **Squared Distance** — when enabled, the output holds the distance **squared** instead of the distance itself. Squaring avoids a square-root step and keeps integer arithmetic exact, but the output is no longer in linear distance units. Leave off for true distances.
- **Use Image Spacing** — when enabled, distances are measured in the image's **physical units** (using the pixel spacing of the geometry). When disabled, distances are measured in **pixels**.

### Required Input Sources

Requires an image whose pixels encode objects (any nonzero value is part of an object; zero is background). This is commonly a segmentation mask or label image produced upstream.

## See Also

- [ITK Signed Danielsson Distance Map Image Filter](ITKSignedDanielssonDistanceMapImageFilter.md)
- [ITK Danielsson Distance Map Image Filter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1DanielssonDistanceMapImageFilter.html)

Reference: Danielsson, Per-Erik. *Euclidean Distance Mapping.* Computer Graphics and Image Processing 14, 227-248 (1980).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

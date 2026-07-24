# ITK Iso Contour Distance Image Filter

For a level-set (signed) image, estimates how far each grid pixel near the zero contour is from that contour.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

This is a specialized filter used in **level-set** methods. A level-set image is a signed image whose zero crossing (the **iso-contour** at value 0) implicitly represents a curve or surface — pixels inside the shape are negative and pixels outside are positive. As a level-set evolves, the values drift away from a true distance and must periodically be "reinitialized" back to a clean signed-distance field.

This filter performs the first step of that reinitialization: for the grid pixels lying immediately next to the iso-contour (those for which the contour crosses a segment to a direct neighbor), it computes an accurate estimate of the distance from the pixel to the interpolated contour. Pixels farther from the contour are left at a saturated "far" value. If a narrowband is supplied, only pixels within that band are processed.

This filter is for users implementing level-set / fast-marching workflows; for a general-purpose distance transform of a binary mask use one of the distance-map filters such as [ITK Signed Maurer Distance Map Image Filter](ITKSignedMaurerDistanceMapImageFilter.md).

### Parameter Guidance

- **Level Set Value** — the iso-value whose contour is located, in the **input image's intensity units** (default *0*, the usual zero level set).
- **Far Value** — the saturation distance assigned to pixels that are not adjacent to the contour. Pixels beyond the immediate neighborhood of the contour are clamped to this value (default *10*), so it should be set larger than the band width you care about. (Units are the same length units as the computed distances.)

### Required Input Sources

Operates on a signed scalar (level-set) image — typically the output of a prior level-set or distance-map filter.

## Reference

K. Krissian and C.-F. Westin, "Fast and Accurate Redistancing for Level Set Methods," EUROCAST NeuroImaging Workshop, Ninth International Conference on Computer Aided Systems Theory, pp. 48-51, Feb 2003.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

# ITK Morphological Watershed From Markers Image Filter

Marker-controlled watershed segmentation: floods the image from user-supplied seeds so that each seed produces exactly one labeled region.

## Group (Subgroup)

ITKWatersheds (Watersheds)

## Description

The **watershed transform** treats a grayscale image as a terrain (bright = high ground, dark = low ground) and floods it from its low points, partitioning it into **catchment basins** separated by **watershed lines**. Run on its own, a watershed almost always produces far more regions than there are real objects, because every small dip seeds a basin (over-segmentation).

The **marker-controlled** variant fixes this. Instead of seeding a basin at every local minimum, it floods only from a set of **markers** — labeled seed points the user places, typically one marker per object of interest plus one for the background. Exactly one output region is produced per marker, and each output region carries that marker's label. Choosing good markers (for example, the centers of objects found by thresholding and [ITK Connected Component Image Filter](ITKConnectedComponentImageFilter.md)) is what makes this approach robust.

> **Not currently available:** in ITK this filter takes *two* images — the image to flood (usually a gradient/edge image) and a separate **marker (seed) label image**. The DREAM3D-NX wrapper only exposes a single input array (no marker-image parameter), so it is **not built into the application** at present. See [bluequartzsoftware/simplnx#1639](https://github.com/BlueQuartzSoftware/simplnx/issues/1639). For an unseeded watershed, use [ITK Morphological Watershed Image Filter](ITKMorphologicalWatershedImageFilter.md).

### Parameter Guidance

- **Mark Watershed Line** — when on, the boundary pixels between regions are written as 0; when off, no separating line is drawn.
- **Fully Connected** — controls neighbor connectivity (face-only versus face + edge + corner). Turn it on for thin, one-pixel-wide features.

The output is a label image whose region labels come from the marker labels. Compare with the unseeded [ITK Morphological Watershed Image Filter](ITKMorphologicalWatershedImageFilter.md).

## Reference

Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003 (Chapter 9.2). Beare R., Lehmann G., "The watershed transform in ITK — discussion and new developments," Insight Journal, <https://www.insight-journal.org/browse/publication/92>.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

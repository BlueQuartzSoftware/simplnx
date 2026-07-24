# ITK Morphological Watershed Image Filter

Segments an image into labeled regions using the watershed transform — flooding the image as if it were a terrain.

## Group (Subgroup)

ITKWatersheds (Watersheds)

## Description

The **watershed transform** treats a grayscale image as a topographic surface: bright pixels are high ground and dark pixels are low ground. Imagine rain falling on this terrain and water collecting in the low spots. Each local low point seeds a **catchment basin**, and the rising water floods outward until basins meet. The lines where neighboring basins meet are the **watershed lines** (the ridge tops between valleys). Each basin becomes one labeled output region.

This filter is most commonly run on a gradient/edge image (for example the output of [ITK Morphological Gradient Image Filter](ITKMorphologicalGradientImageFilter.md)), so that bright ridges fall on object boundaries and the basins fall on the objects.

The output is a **label image**: every region has a unique integer value, and the watershed lines (if marked) are 0. A raw watershed almost always produces **far more regions than there are real objects** (over-segmentation) because every small dip becomes its own basin; the *Level* parameter and a marker-controlled variant exist to control this.

### Parameter Guidance

- **Level** — a depth/contrast threshold, in the **input image's intensity units**, that merges shallow neighboring basins. A higher Level merges more aggressively and yields fewer, larger regions; *0* (the default) performs no merging and tends to over-segment. Raising Level is the primary way to reduce over-segmentation.
- **Mark Watershed Line** — when on, the boundary pixels between regions are written as 0; when off, no separating line is drawn.
- **Fully Connected** — controls neighbor connectivity (face-only versus face + edge + corner). Turn it on for thin, one-pixel-wide features.

The output labels are in no particular order. To renumber them consecutively and sort by region size, follow this filter with [ITK Relabel Component Image Filter](ITKRelabelComponentImageFilter.md). A marker-controlled variant (one region per user-supplied seed, avoiding over-segmentation) exists in ITK but is not currently built into DREAM3D-NX (see [issue #1639](https://github.com/BlueQuartzSoftware/simplnx/issues/1639)).

### Required Input Sources

Operates on any scalar (typically gradient/edge) image — for example the output of [ITK Morphological Gradient Image Filter](ITKMorphologicalGradientImageFilter.md) or another ITK image filter.

## Reference

Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003 (Chapter 9.2). Beare R., Lehmann G., "The watershed transform in ITK — discussion and new developments," Insight Journal, <https://www.insight-journal.org/browse/publication/92>.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

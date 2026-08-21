# ITK Label Contour Image Filter

From a label image, keeps only the boundary pixels of each labeled region.

## Group (Subgroup)

ITKImageLabel (ImageLabel)

## Description

This filter takes a **label image** (one where each region has a unique integer value) and keeps only the pixels that lie on the contour (outer boundary) of each labeled region. Object pixels are those whose value differs from the *Background Value*. Pixels that sit on a region's border keep their original label, while all interior pixels are set to the *Background Value*. The result is an outline of every region, with each outline retaining the label of the region it bounds.

Use this filter when you need the edges of segmented regions — for example to overlay region boundaries on an image or to measure perimeters — rather than the filled regions themselves.

### Parameter Guidance

- **Background Value** — the pixel value treated as background. Any pixel equal to this value is ignored, and interior (non-boundary) object pixels are reset to it. The default is 0.
- **Fully Connected** — controls connectivity. *Off* uses face-only connectivity. *On* uses face+edge+corner connectivity, which produces thicker contours. Turn this *On* for objects that are 1 pixel wide or that connect along diagonals.

### Required Input Sources

A label image (cell data on an **Image Geometry**), such as the output of the [ITK Connected Component Image Filter](ITKConnectedComponentImageFilter.md) or a segmentation filter.

% Auto generated parameter table will be inserted here

## See Also

- [Insight Journal: Label object representation and manipulation with ITK](https://www.insight-journal.org/browse/publication/217) — Gaetan Lehmann, INRA de Jouy-en-Josas, France.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

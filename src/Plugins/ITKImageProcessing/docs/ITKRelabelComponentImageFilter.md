# ITK Relabel Component Image Filter

Renumbers the labels in a label image consecutively, sorted so that label 1 is the largest object.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

This filter takes a **label image** (one where each region has a unique integer value) and remaps the labels so they are consecutive with no gaps. By default it also sorts the labels by object size: the largest object becomes label 1, the second largest becomes label 2, and so on. If two objects have the same size their original relative order is kept. Sorting can be turned off, in which case the original ordering of the labels is preserved while still removing any gaps.

Label 0 is assumed to be the background and is left unchanged.

Optionally, objects smaller than a minimum size can be discarded. This is useful for cleaning up small spurious regions after a connected-components step. Because the labels are sorted by size, you can then easily extract the largest object (label 1) or the *k* largest objects.

Use this filter on the output of a labeling step, such as the [ITK Connected Component Image Filter](ITKConnectedComponentImageFilter.md) or a watershed segmentation, when you want labels ordered by size or want to drop small objects.

### Parameter Guidance

- **Minimum Object Size** — the smallest object to keep, measured as a **voxel/pixel count** (number of pixels in the object). Any object with fewer pixels than this value is discarded and does not appear in the output. A value of 0 keeps every object.
- **SortByObjectSize** — when *On* (the default), labels are renumbered in order of decreasing object size (label 1 = largest). When *Off*, the original label order is preserved and only the gaps are removed.

### Required Input Sources

A label image (cell data on an **Image Geometry**), such as the output of the [ITK Connected Component Image Filter](ITKConnectedComponentImageFilter.md) or a watershed/segmentation filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

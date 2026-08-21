# ITK Connected Component Image Filter

Labels each connected object in a binary image with a unique integer.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

This filter performs **connected components** labeling: it labels each group of touching non-zero pixels with a unique consecutive integer (0 = background). Non-zero pixels are treated as object material and zero-valued pixels as background. Every distinct touching cluster of object pixels receives its own integer label, so the output is a **label image** (one where each region has a unique integer value).

Labels are assigned consecutively starting at 1, with no gaps. The label numbers follow the raster scan order in which objects are first encountered, so the assigned numbers carry no meaning beyond identity. To order the labels by object size instead, pass the output to the [ITK Relabel Component Image Filter](ITKRelabelComponentImageFilter.md), which renumbers them so that label 1 is the largest object.

Use this filter when you have a thresholded (binary) image and need to identify and count the individual objects within it.

### Parameter Guidance

- **Fully Connected** — controls connectivity. *Off* uses face-only connectivity (pixels touch only across shared faces). *On* uses face+edge+corner connectivity, so objects that touch only diagonally are merged into one. Turn this *On* for objects that are 1 pixel wide or that connect along diagonals.

### Required Input Sources

A binary image (cell data on an **Image Geometry**) where object pixels are non-zero and background pixels are zero, typically produced by a thresholding filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

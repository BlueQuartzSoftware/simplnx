# Otsu Multiple Thresholds Image Filter

Threshold an image using multiple Otsu Thresholds.

## Group (Subgroup)

ImageProcessing (Thresholding)

## Description

This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using a multi-Otsu calculator and applies those thresholds to the input image to produce class labels. The **Number Of Histogram Bins** and **Number Of Thresholds** control the calculator. The **Label Offset** is added to every output label so labels can start from a value other than zero.

Each output voxel is assigned the index of the class its intensity falls into: the number of thresholds the value exceeds, plus the **Label Offset**. The output is always a `uint8` label image, regardless of the (numeric, single-component) input element type.

This filter also includes an option to use the valley emphasis algorithm from H.F. Ng, "Automatic thresholding for defect detection", Pattern Recognition Letters, (27): 1644-1649, 2006. The valley emphasis algorithm is particularly effective when the object to be thresholded is small. It is turned off by default; enable it with **Valley Emphasis**.

ITK-free, out-of-core-capable reimplementation of the legacy ITK Otsu Multiple Thresholds Image Filter. The histogram and input range are computed with bounded-memory streaming passes, so memory use stays bounded regardless of image size.

## Algorithm

For integral inputs whose observed intensity span is at most 65,535, the filter determines the input range and exact value frequencies in one global scan using a fixed 65,536-entry frequency ring, then populates the requested histogram without rereading the image. Wider integral ranges fall back to a second streaming scan after the range is known. Floating-point inputs retain the statistics scan followed by the histogram scan so their established NaN and range behavior remains unchanged.

The thresholds are calculated from the fixed-size histogram and applied independently to every voxel. Disk-backed global scans use byte-capped buffers of at most 64 MiB, the frequency ring has a fixed size independent of the image volume, and the output pass uses separate bounded input and output buffers. Store transfers remain serial and memory use does not grow with the image volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

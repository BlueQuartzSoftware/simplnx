# Normalize To Constant Image Filter

Scales image pixel intensities so that the sum of all pixels equals a user-defined constant.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the streaming sum of the input image, then scales every pixel so that the new sum equals **Constant** (default 1.0):

```
outputPixel = inputPixel * (Constant / sum(input))
```

This is especially useful for normalizing a convolution kernel. The output image always has a `float64` element type, regardless of the input element type. If the sum of the input is zero, the filter fails at execution with an error rather than dividing by zero. ITK-free, out-of-core-capable reimplementation of the legacy ITK Normalize To Constant Image Filter.

## Algorithm

The filter first computes the global sum in input order, then scales every value independently. Disk-backed inputs are read sequentially in byte-capped chunks of at most 64 MiB for the statistics pass; the output pass uses separate bounded input and output buffers. Store transfers remain serial and memory use does not grow with the image volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

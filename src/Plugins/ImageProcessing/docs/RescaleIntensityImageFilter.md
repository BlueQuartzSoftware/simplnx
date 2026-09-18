# Rescale Intensity Image Filter

Applies a linear transformation to the intensity levels of the input image so that the minimum and maximum values map to user-defined output bounds.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Computes the minimum and maximum intensity of the input image (a streaming statistics pass), then applies a linear transformation so that the input minimum maps to **Output Minimum** and the input maximum maps to **Output Maximum**:

```
outputPixel = (inputPixel - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin
```

If the input image has a constant value (`inputMin == inputMax`), the filter falls back to scaling by the input maximum alone (or leaves values unchanged if the input maximum is zero). ITK-free, out-of-core-capable reimplementation of the legacy ITK Rescale Intensity Image Filter.

## Algorithm

The filter first computes the global minimum and maximum, then applies the rescaling independently to every value. Resident arrays use a parallel extrema reduction. Disk-backed inputs are read sequentially in byte-capped chunks of at most 64 MiB for the extrema pass; the output pass uses separate bounded input and output buffers. Store transfers remain serial and memory use does not grow with the image volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

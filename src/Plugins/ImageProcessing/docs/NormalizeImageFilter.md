# Normalize Image Filter

Normalizes an image by setting its mean to zero and variance to one.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Shifts and scales the input image so that the output pixels have zero mean and unit variance, using the streaming mean and sample (N-1) variance of the input:

```
outputPixel = (inputPixel - mean) / sigma
```

The output image always has a `float64` element type, regardless of the input element type. Note that when the output is cast back to an integral type, roughly 68% of the intensity range maps to `[-1, 1]`, so an integral output will not actually have unit variance. ITK-free, out-of-core-capable reimplementation of the legacy ITK Normalize Image Filter.

## Algorithm

The filter first computes the global mean and sample variance, then applies the normalization independently to every value. Resident arrays use a deterministic parallel reduction. Disk-backed inputs are read sequentially in byte-capped chunks of at most 64 MiB for the statistics pass; the output pass uses separate bounded input and output buffers. Store transfers remain serial and memory use does not grow with the image volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

# Mask Image Filter

Masks an image using a second image as the mask.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

For every tuple, if the corresponding mask value is nonzero, the output pixel is a copy of the input pixel (all components); otherwise every component of the output pixel is set to **Outside Value**. The mask array must be `uint8`, `uint16`, or `uint32`, and must have the same number of tuples as the input array. The output element type matches the input element type. ITK-free, out-of-core-capable reimplementation of the legacy ITK Mask Image Filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

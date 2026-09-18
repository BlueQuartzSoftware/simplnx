# Binary Threshold Image Filter

Binarize an input image by thresholding.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

This filter produces an output image whose pixels are either one of two values (**Outside Value** or **Inside Value**), depending on whether the corresponding input image pixels lie between the two thresholds (**Lower Threshold** and **Upper Threshold**). Values equal to either threshold are considered to be between the thresholds. The output array is always `uint8`, regardless of the input element type. ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Threshold Image Filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

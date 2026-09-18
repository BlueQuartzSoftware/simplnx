# Intensity Windowing Image Filter

Applies a linear transformation to the intensity levels of the input image.

## Group (Subgroup)

ImageProcessing (Pointwise)

## Description

Applies a linear transformation to the intensity levels of the input image that are inside a user-defined window (**Window Minimum**, **Window Maximum**), mapping them to a new user-defined range (**Output Minimum**, **Output Maximum**). Values below **Window Minimum** are mapped to **Output Minimum**, and values above **Window Maximum** are mapped to **Output Maximum**. Values inside the window are linearly rescaled. ITK-free, out-of-core-capable reimplementation of the legacy ITK Intensity Windowing Image Filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

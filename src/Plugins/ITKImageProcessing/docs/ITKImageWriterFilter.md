# Write Image (ITK)

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

This **Filter** will save images based on an array that represents grayscale, RGB or ARGB color values. If the input array represents a 3D volume, the **Filter** will output a series of slices along one of the orthogonal axes.  The options are to produce XY slices along the Z axis, XZ slices along the Y axis or YZ slices along the X axis. The user has the option to save in one of 3 standard image formats: TIF, BMP, or PNG. The output files will be numbered sequentially starting at zero (0) and ending at the total dimensions for the chosen axis. For example, if the Z axis has 117 dimensions, 117 XY image files will be produced and numbered 0 to 116. Unless the data is a single slice then only a single image will be produced using the name given in the Output File parameter.

An example of a **Filter** that produces color data that can be used as input to this **Filter**
is the {ref}`Generate IFP Colors <OrientationAnalysis/ComputeIPFColorsFilter:Description>` **Filter**, which will generate RGB values for each voxel in the volume.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ INL Export
+ TxCopper_Exposed
+ TxCopper_Unexposed
+ Edax IPF Colors

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

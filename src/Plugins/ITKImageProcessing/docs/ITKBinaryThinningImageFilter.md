# ITK Binary Thinning Image Filter

This filter computes one-pixel-wide edges of the input image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

This class is parameterized over the type of the input image and the type of the output image.

The input is assumed to be a binary image. If the foreground pixels of the input image do not have a value of 1, they are rescaled to 1 internally to simplify the computation.

The filter will produce a skeleton of the object. The output background values are 0, and the foreground values are 1.

This filter is a sequential thinning algorithm and known to be computational time dependable on the image size. The algorithm corresponds with the 2D implementation described in:

Rafael C. Gonzales and Richard E. Woods. Digital Image Processing. Addison Wesley, 491-494, (1993).

To do: Make this filter ND.* MorphologyImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

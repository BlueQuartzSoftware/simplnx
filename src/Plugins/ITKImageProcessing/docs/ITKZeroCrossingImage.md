# ITK Zero Crossing Image Filter (ITKZeroCrossingImage)

This filter finds the closest pixel to the zero-crossings (sign changes) in a signed itk::Image .

## Group (Subgroup)

ITKImageFeature (ImageFeature)

## Description

Pixels closest to zero-crossings are labeled with a foreground value. All other pixels are marked with a background value. The algorithm works by detecting differences in sign among neighbors using city-block style connectivity (4-neighbors in 2d, 6-neighbors in 3d, etc.).

## Parameter Information

- The input to this filter is an itk::Image of arbitrary dimension. The algorithm assumes a signed data type (zero-crossings are not defined for unsigned int data types), and requires that operator>, operator<, operator==, and operator!= are defined.

- The output of the filter is a binary, labeled image of user-specified type. By default, zero-crossing pixels are labeled with a default "foreground" value of itk::NumericTraits<OutputDataType>::OneValue() , where OutputDataType is the data type of the output image. All other pixels are labeled with a default "background" value of itk::NumericTraits<OutputDataType>::ZeroValue() .

- There are two parameters for this filter. ForegroundValue is the value that marks zero-crossing pixels. The BackgroundValue is the value given to all other pixels.

## See Also

- [Image](https://itk.org/Doxygen/html/classitk_1_1Image.html)

- [Neighborhood](https://itk.org/Doxygen/html/classitk_1_1Neighborhood.html)

- [NeighborhoodOperator](https://itk.org/Doxygen/html/classitk_1_1NeighborhoodOperator.html)

- [NeighborhoodIterator](https://itk.org/Doxygen/html/classitk_1_1NeighborhoodIterator.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

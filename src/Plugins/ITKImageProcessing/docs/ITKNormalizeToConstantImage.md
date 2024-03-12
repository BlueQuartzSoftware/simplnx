# ITK Normalize To Constant Image Filter (ITKNormalizeToConstantImage)

Scales image pixel intensities to make the sum of all pixels equal a user-defined constant.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

The default value of the constant is 1. It can be changed with SetConstant() .

This transform is especially useful for normalizing a convolution kernel.

This code was contributed in the Insight Journal paper: ["FFT based
convolution" by Lehmann G. https://insight-journal.org/browse/publication/717](https://insight-journal.org/browse/publication/717)

## Author

- Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

## See Also

- [NormalizeImageFilter](https://itk.org/Doxygen/html/classitk_1_1NormalizeImageFilter.html)

- [StatisticsImageFilter](https://itk.org/Doxygen/html/classitk_1_1StatisticsImageFilter.html)

- [DivideImageFilter](https://itk.org/Doxygen/html/classitk_1_1DivideImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

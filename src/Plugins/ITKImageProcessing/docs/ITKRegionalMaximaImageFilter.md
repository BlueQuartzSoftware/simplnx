# ITK Regional Maxima Image Filter

Produce a binary image where foreground is the regional maxima of the input image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional maxima are flat zones surrounded by pixels of lower value.

If the input image is constant, the entire image can be considered as a maxima or not. The desired behavior can be selected with the SetFlatIsMaxima() method.

## Author

This class was contributed to the Insight Journal by author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France. The paper can be found at https://www.insight-journal.org/browse/publication/65

## See Also

- [ValuedRegionalMaximaImageFilter](https://itk.org/Doxygen/html/classitk_1_1ValuedRegionalMaximaImageFilter.html)

- [HConvexImageFilter](https://itk.org/Doxygen/html/classitk_1_1HConvexImageFilter.html)

- [RegionalMinimaImageFilter](https://itk.org/Doxygen/html/classitk_1_1RegionalMinimaImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

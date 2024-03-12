# ITK Regional Minima Image Filter (ITKRegionalMinimaImage)

Produce a binary image where foreground is the regional minima of the input image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional minima are flat zones surrounded by pixels of greater value.

If the input image is constant, the entire image can be considered as a minima or not. The SetFlatIsMinima() method let the user choose which behavior to use.

This class was contributed to the Insight Journal by \author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France. https://www.insight-journal.org/browse/publication/65 


\see RegionalMaximaImageFilter 


\see ValuedRegionalMinimaImageFilter 


\see HConcaveImageFilter


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

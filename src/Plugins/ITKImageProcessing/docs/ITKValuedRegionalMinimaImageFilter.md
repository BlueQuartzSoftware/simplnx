# ITK Valued Regional Minima Image Filter

Transforms the image so that any pixel that is not a regional minima is set to the maximum value for the pixel type. Pixels that are regional minima retain their value.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional minima are flat zones surrounded by pixels of higher value. A completely flat image will be marked as a regional minima by this filter.

This code was contributed in the Insight Journal paper: "Finding regional extrema - methods and performance" by Beare R., Lehmann G. <https://www.insight-journal.org/browse/publication/65>

### Author

 Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.

### Related Filters

- ValuedRegionalMaximaImageFilter , ValuedRegionalExtremaImageFilter ,
- HMinimaImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

# ITK Valued Regional Maxima Image Filter

Transforms the image so that any pixel that is not a regional maxima is set to the minimum value for the pixel type. Pixels that are regional maxima retain their value.

The pixel neighborhood is controlled by the FullyConnected attribute. All adjacent pixels are included in
the neighborhood when FullyConnected=True and the diagonally adjacent pixels are not included when
FullyConnected=False. Different terminology is often used to describe neighborhoods – one common ex-
ample is the “connectivity” notation, which refers to the number of pixels in the neighborhood. FullyCon-
nected=False corresponds to a connectivty of 4 in 2D and 6 in 3D, while FullyConnected=True corresponds
to a connectivity of 8 in 2D and 26 in 3D. FullyConnected=False is also commonly referred to as “face
connected”.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional maxima are flat zones surrounded by pixels of lower value. A completely flat image will be marked as a regional maxima by this filter.

This code was contributed in the Insight Journal paper: "Finding regional extrema - methods and performance" by Beare R., Lehmann G. <https://www.insight-journal.org/browse/publication/65>

### Author

 Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.

### Related Filters

- ValuedRegionalMinimaImageFilter
- ValuedRegionalExtremaImageFilter
- HMinimaImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

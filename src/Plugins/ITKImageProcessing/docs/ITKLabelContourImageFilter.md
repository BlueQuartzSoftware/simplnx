# ITK Label Contour Image Filter

Labels the pixels on the border of the objects in a labeled image.

## Group (Subgroup)

ITKImageLabel (ImageLabel)

## Description

LabelContourImageFilter takes a labeled image as input, where the pixels in the objects are the pixels with a value different of the BackgroundValue. Only the pixels on the contours of the objects are kept. The pixels not on the border are changed to BackgroundValue. The labels of the object are the same in the input and in the output image.

The connectivity can be changed to minimum or maximum connectivity with SetFullyConnected() . Full connectivity produces thicker contours.

<https://www.insight-journal.org/browse/publication/217>

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- BinaryContourImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

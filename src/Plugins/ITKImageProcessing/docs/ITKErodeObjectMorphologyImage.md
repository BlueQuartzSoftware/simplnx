# ITK Erode Object Morphology Image Filter

Erosion of an object in an image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

Erosion of an image using binary morphology. Pixel values matching the object value are considered the "object" and all other pixels are "background". This is useful in processing mask images containing only one object.

If the pixel covered by the center of the kernel has the pixel value ObjectValue and the pixel is adjacent to a non-object valued pixel, then the kernel is centered on the object-value pixel and neighboring pixels covered by the kernel are assigned the background value. The structuring element is assumed to be composed of binary values (zero or one).* ObjectMorphologyImageFilter , BinaryFunctionErodeImageFilter

- BinaryErodeImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

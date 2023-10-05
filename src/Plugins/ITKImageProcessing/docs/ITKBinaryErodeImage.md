# ITK Binary Erode Image Filter

Fast binary erosion of a single intensity value in the image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

BinaryErodeImageFilter is a binary erosion morphologic operation on the foreground of an image. Only the value designated by the intensity value "SetForegroundValue()" (alias as SetErodeValue() ) is considered as foreground, and other intensity values are considered background.

Grayscale images can be processed as binary images by selecting a "ForegroundValue" (alias "ErodeValue"). Pixel values matching the erode value are considered the "foreground" and all other pixels are "background". This is useful in processing segmented images where all pixels in segment #1 have value 1 and pixels in segment #2 have value 2, etc. A particular "segment number" can be processed. ForegroundValue defaults to the maximum possible value of the PixelType. The eroded pixels will receive the BackgroundValue (defaults to NumericTraits::NonpositiveMin() ).

The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel. A reasonable choice of structuring element is itk::BinaryBallStructuringElement .

This implementation is based on the papers:

L.Vincent "Morphological transformations of binary images with
arbitrary structuring elements", and

N.Nikopoulos et al. "An efficient algorithm for 3d binary
morphological transformations with 3d structuring elements
for arbitrary size and shape". IEEE Transactions on Image Processing. Vol. 9. No. 3. 2000. pp. 283-286.* ImageToImageFilter BinaryDilateImageFilter BinaryMorphologyImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

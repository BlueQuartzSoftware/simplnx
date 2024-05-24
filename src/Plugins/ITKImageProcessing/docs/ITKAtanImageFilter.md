# ITK Atan Image Filter

Computes the one-argument inverse tangent of each pixel.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This filter is templated over the pixel type of the input image and the pixel type of the output image.

The filter walks over all the pixels in the input image, and for each pixel does the following:

- cast the pixel value to double ,
- apply the std::atan() function to the double value,
- cast the double value resulting from std::atan() to the pixel type of the output image,
- store the cast value into the output image.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

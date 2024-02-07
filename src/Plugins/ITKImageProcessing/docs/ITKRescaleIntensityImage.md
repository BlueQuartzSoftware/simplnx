# ITK Rescale Intensity Image Filter

Applies a linear transformation to the intensity levels of the input Image .

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have.

The following equation gives the mapping of the intensity values

 \f[ outputPixel = ( inputPixel - inputMin) \cdot \frac{(outputMax - outputMin )}{(inputMax - inputMin)} + outputMin \f]

All computations are performed in the precision of the input pixel's RealType. Before assigning the computed value to the output pixel.

NOTE: In this filter the minimum and maximum values of the input image are computed internally using the MinimumMaximumImageCalculator . Users are not supposed to set those values in this filter. If you need a filter where you can set the minimum and maximum values of the input, please use the IntensityWindowingImageFilter . If you want a filter that can use a user-defined linear transformation for the intensity, then please use the ShiftScaleImageFilter .* IntensityWindowingImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

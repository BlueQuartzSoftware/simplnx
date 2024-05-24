# ITK Connected Component Image Filter

Label the objects in a binary image.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

ConnectedComponentImageFilter labels the objects in a binary image (non-zero pixels are considered to be objects, zero-valued pixels are considered to be background). Each distinct object is assigned a unique label. The filter experiments with some improvements to the existing implementation, and is based on run length encoding along raster lines. If the output background value is set to zero (the default), the final object labels start with 1 and are consecutive. If the output background is set to a non-zero value (by calling the SetBackgroundValue() routine of the filter), the final labels start at 0, and remain consecutive except for skipping the background value as needed. Objects that are reached earlier by a raster order scan have a lower label. This is different to the behaviour of the original connected component image filter which did not produce consecutive labels or impose any particular ordering.

After the filter is executed, ObjectCount holds the number of connected components.

## See Also

- [ImageToImageFilter](https://itk.org/Doxygen/html/classitk_1_1ImageToImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

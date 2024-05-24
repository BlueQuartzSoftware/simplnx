# ITK Threshold Maximum Connected Components Image Filter

Finds the threshold value of an image based on maximizing the number of objects in the image that are larger than a given minimal size.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

\par 
This method is based on Topological Stable State Thresholding to calculate the threshold set point. This method is particularly effective when there are a large number of objects in a microscopy image. Compiling in Debug mode and enable the debug flag for this filter to print debug information to see how the filter focuses in on a threshold value. Please see the Insight Journal's MICCAI 2005 workshop for a complete description. References are below.


\par Parameters
The MinimumObjectSizeInPixels parameter is controlled through the class Get/SetMinimumObjectSizeInPixels() method. Similar to the standard itk::BinaryThresholdImageFilter the Get/SetInside and Get/SetOutside values of the threshold can be set. The GetNumberOfObjects() and GetThresholdValue() methods return the number of objects above the minimum pixel size and the calculated threshold value.


\par Automatic Thresholding in ITK
There are multiple methods to automatically calculate the threshold intensity value of an image. As of version 4.0, ITK has a Thresholding ( ITKThresholding ) module which contains numerous automatic thresholding methods.implements two of these. Topological Stable State Thresholding works well on images with a large number of objects to be counted.

## References

1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber
counting in immunofluorescent microscopy images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) https://insight-journal.org/browse/publication/40 2) Pikaz A, Averbuch, A. "Digital image thresholding based on topological
stable-state". Pattern Recognition, 29(5): 829-843, 1996.

## Questions

email Ken Urish at ken.urish(at)gmail.com Please cc the itk list serve for archival purposes.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

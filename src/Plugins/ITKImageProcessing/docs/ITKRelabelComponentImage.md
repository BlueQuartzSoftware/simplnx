# ITK::Relabel Component Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Relabel the components in an image such that consecutive labels are used.

RelabelComponentImageFilter remaps the labels associated with the objects in an image (as from the output of ConnectedComponentImageFilter ) such that the label numbers are consecutive with no gaps between the label numbers used. By default, the relabeling will also sort the labels based on the size of the object: the largest object will have label #1, the second largest will have label #2, etc. If two labels have the same size their initial order is kept. The sorting by size can be disabled using SetSortByObjectSize.

Label #0 is assumed to be the background and is left unaltered by the relabeling.

RelabelComponentImageFilter is typically used on the output of the ConnectedComponentImageFilter for those applications that want to extract the largest object or the "k" largest objects. Any particular object can be extracted from the relabeled output using a BinaryThresholdImageFilter . A group of objects can be extracted from the relabled output using a ThresholdImageFilter .

Once all the objects are relabeled, the application can query the number of objects and the size of each object. Object sizes are returned in a vector. The size of the background is not calculated. So the size of object #1 is GetSizeOfObjectsInPixels() [0], the size of object #2 is GetSizeOfObjectsInPixels() [1], etc.

If user sets a minimum object size, all objects with fewer pixels than the minimum will be discarded, so that the number of objects reported will be only those remaining. The GetOriginalNumberOfObjects method can be called to find out how many objects were present before the small ones were discarded.

RelabelComponentImageFilter can be run as an "in place" filter, where it will overwrite its output. The default is run out of place (or generate a separate output). "In place" operation can be controlled via methods in the superclass, InPlaceImageFilter::InPlaceOn() and InPlaceImageFilter::InPlaceOff() .

\see ConnectedComponentImageFilter , BinaryThresholdImageFilter , ThresholdImageFilter

\par Wiki Examples:

\li All Examples

\li Assign contiguous labels to connected regions of an image

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| MinimumObjectSize | double| Set the minimum size in pixels for an object. All objects smaller than this size will be discarded and will not appear in the output label map. NumberOfObjects will count only the objects whose pixel counts are greater than or equal to the minimum size. Call GetOriginalNumberOfObjects to find out how many objects were present in the original label map. |
| SortByObjectSize | bool| Controls whether the object labels are sorted by size. If false, initial order of labels is kept. |
| NumberOfObjects | double| Get the number of objects in the image. This information is only valid after the filter has executed. |
| OriginalNumberOfObjects | double| Get the original number of objects in the image before small objects were discarded. This information is only valid after the filter has executed. If the caller has not specified a minimum object size, OriginalNumberOfObjects is the same as NumberOfObjects. |
| SizeOfObjectsInPhysicalUnits | FloatVec3_t| Get the size of each object in physical space (in units of pixel size). This information is only valid after the filter has executed. Size of the background is not calculated. Size of object #1 is GetSizeOfObjectsInPhysicalUnits() [0]. Size of object #2 is GetSizeOfObjectsInPhysicalUnits() [1]. Etc. |
| SizeOfObjectsInPixels | FloatVec3_t| Get the size of each object in pixels. This information is only valid after the filter has executed. Size of the background is not calculated. Size of object #1 is GetSizeOfObjectsInPixels() [0]. Size of object #2 is GetSizeOfObjectsInPixels() [1]. Etc. |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None |  | (1)  | Array containing filtered image

## References ##

[1] T.S. Yoo, M. J. Ackerman, W. E. Lorensen, W. Schroeder, V. Chalana, S. Aylward, D. Metaxas, R. Whitaker. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK - The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002). 
[2] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Design and Functionality. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-28-3
[3] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Introduction and Development Guidelines. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-27-6

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users

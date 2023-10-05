# ITK Relabel Component Image Filter

Relabel the components in an image such that consecutive labels are used.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

RelabelComponentImageFilter remaps the labels associated with the objects in an image (as from the output of ConnectedComponentImageFilter ) such that the label numbers are consecutive with no gaps between the label numbers used. By default, the relabeling will also sort the labels based on the size of the object: the largest object will have label #1, the second largest will have label #2, etc. If two labels have the same size their initial order is kept. The sorting by size can be disabled using SetSortByObjectSize.

Label #0 is assumed to be the background and is left unaltered by the relabeling.

RelabelComponentImageFilter is typically used on the output of the ConnectedComponentImageFilter for those applications that want to extract the largest object or the "k" largest objects. Any particular object can be extracted from the relabeled output using a BinaryThresholdImageFilter . A group of objects can be extracted from the relabeled output using a ThresholdImageFilter .

Once all the objects are relabeled, the application can query the number of objects and the size of each object. Object sizes are returned in a vector. The size of the background is not calculated. So the size of object #1 is GetSizeOfObjectsInPixels() [0], the size of object #2 is GetSizeOfObjectsInPixels() [1], etc.

If user sets a minimum object size, all objects with fewer pixels than the minimum will be discarded, so that the number of objects reported will be only those remaining. The GetOriginalNumberOfObjects method can be called to find out how many objects were present before the small ones were discarded.

RelabelComponentImageFilter can be run as an "in place" filter, where it will overwrite its output. The default is run out of place (or generate a separate output). "In place" operation can be controlled via methods in the superclass, InPlaceImageFilter::InPlaceOn() and InPlaceImageFilter::InPlaceOff().* ConnectedComponentImageFilter , BinaryThresholdImageFilter , ThresholdImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

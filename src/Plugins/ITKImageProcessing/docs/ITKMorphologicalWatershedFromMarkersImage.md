# ITK Morphological Watershed From Markers Image Filter (ITKMorphologicalWatershedFromMarkersImage)

Morphological watershed transform from markers.

## Group (Subgroup)

ITKWatersheds (Watersheds)

## Description

The watershed transform is a tool for image segmentation that is fast and flexible and potentially fairly parameter free. It was originally derived from a geophysical model of rain falling on a terrain and a variety of more formal definitions have been devised to allow development of practical algorithms. If an image is considered as a terrain and divided into catchment basins then the hope is that each catchment basin would contain an object of interest.

The output is a label image. A label image, sometimes referred to as a categorical image, has unique values for each region. For example, if a watershed produces 2 regions, all pixels belonging to one region would have value A, and all belonging to the other might have value B. Unassigned pixels, such as watershed lines, might have the background value (0 by convention).

The simplest way of using the watershed is to preprocess the image we want to segment so that the boundaries of our objects are bright (e.g apply an edge detector) and compute the watershed transform of the edge image. Watershed lines will correspond to the boundaries and our problem will be solved. This is rarely useful in practice because there are always more regional minima than there are objects, either due to noise or natural variations in the object surfaces. Therefore, while many watershed lines do lie on significant boundaries, there are many that don't. Various methods can be used to reduce the number of minima in the image, like thresholding the smallest values, filtering the minima and/or smoothing the image.

This filter use another approach to avoid the problem of over segmentation: it let the user provide a marker image which mark the minima in the input image and give them a label. The minima are imposed in the input image by the markers. The labels of the output image are the label of the marker image.

The morphological watershed transform algorithm is described in Chapter 9.2 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.

This code was contributed in the Insight Journal paper: "The watershed transform in ITK - discussion and new developments" by Beare R., Lehmann G. https://www.insight-journal.org/browse/publication/92 

\author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France. 


\author Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.


\see WatershedImageFilter , MorphologicalWatershedImageFilter


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

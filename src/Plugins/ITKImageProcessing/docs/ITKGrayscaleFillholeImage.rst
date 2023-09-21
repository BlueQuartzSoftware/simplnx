===================================
ITK Grayscale Fillhole Image Filter
===================================


Remove local minima not connected to the boundary of the image.

Group (Subgroup)
================

ITKMathematicalMorphology (MathematicalMorphology)

Description
===========

GrayscaleFillholeImageFilter fills holes in a grayscale image. Holes are local minima in the grayscale topography that
are not connected to boundaries of the image. Gray level values adjacent to a hole are extrapolated across the hole.

This filter is used to smooth over local minima without affecting the values of local maxima. If you take the difference
between the output of this filter and the original image (and perhaps threshold the difference above a small value),
you’ll obtain a map of the local minima.

This filter uses the ReconstructionByErosionImageFilter . It provides its own input as the “mask” input to the geodesic
erosion. The “marker” image for the geodesic erosion is constructed such that boundary pixels match the boundary pixels
of the input image and the interior pixels are set to the maximum pixel value in the input image.

Geodesic morphology and the Fillhole algorithm is described in Chapter 6 of Pierre Soille’s book “Morphological Image
Analysis: Principles and Applications”, Second Edition, Springer, 2003.\* ReconstructionByErosionImageFilter -
MorphologyImageFilter , GrayscaleErodeImageFilter , GrayscaleFunctionErodeImageFilter , BinaryErodeImageFilter

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| FullyConnected            | bool                      | Whether the connected components are defined strictly by    |
|                           |                           | face connectivity (False) or by face+edge+vertex            |
|                           |                           | connectivity (True). Default is False                       |
+---------------------------+---------------------------+-------------------------------------------------------------+

Required Geometry
=================

Image Geometry

Required Objects
================

====================== ======== =================================================================
Name                   Type     Description
====================== ======== =================================================================
Input Image Geometry   DataPath DataPath to the Input Image Geometry
Input Image Data Array DataPath Path to input image with pixel type matching BasicPixelIDTypeList
====================== ======== =================================================================

Created Objects
===============

======================= ======== ==================================================================
Name                    Type     Description
======================= ======== ==================================================================
Output Image Data Array DataPath Path to output image with pixel type matching BasicPixelIDTypeList
======================= ======== ==================================================================

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this plugin.

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.

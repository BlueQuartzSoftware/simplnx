===================================
ITK Gradient Magnitude Image Filter
===================================


Computes the gradient magnitude of an image region at each pixel.

Group (Subgroup)
================

ITKImageGradient (ImageGradient)

Description
===========

Computes the gradient magnitude of an image region at each pixel.

Related Filters
---------------

-  Image
-  Neighborhood
-  NeighborhoodOperator
-  NeighborhoodIterator

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| UseImageSpacing           | bool                      | Set/Get whether or not the filter will use the spacing of   |
|                           |                           | the input image in the computation of the derivatives. Use  |
|                           |                           | On to compute the gradient in physical space; use Off to    |
|                           |                           | ignore image spacing and to compute the gradient in         |
|                           |                           | isotropic voxel space. Default is On.                       |
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

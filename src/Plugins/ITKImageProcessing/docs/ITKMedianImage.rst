=======================
ITK Median Image Filter
=======================


Applies a median filter to an image.

Group (Subgroup)
================

ITKSmoothing (Smoothing)

Description
===========

Computes an image where a given pixel is the median value of the the pixels in a neighborhood about the corresponding
input pixel.

A median filter is one of the family of nonlinear filters. It is used to smooth an image without being biased by
outliers or shot noise.

This filter requires that the input pixel type provides an operator<() (LessThan Comparable).\* Image - Neighborhood -
NeighborhoodOperator - NeighborhoodIterator

Parameters
==========

====== ====== ===========
Name   Type   Description
====== ====== ===========
Radius uint32 
====== ====== ===========

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

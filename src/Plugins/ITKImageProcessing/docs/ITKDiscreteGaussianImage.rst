==================================
ITK Discrete Gaussian Image Filter
==================================


Blurs an image by separable convolution with discrete gaussian kernels. This filter performs Gaussian blurring by
separable convolution of an image and a discrete Gaussian operator (kernel).

Group (Subgroup)
================

ITKSmoothing (Smoothing)

Description
===========

The Gaussian operator used here was described by Tony Lindeberg (Discrete Scale-Space Theory and the Scale-Space Primal
Sketch. Dissertation. Royal Institute of Technology, Stockholm, Sweden. May 1991.) The Gaussian kernel used here was
designed so that smoothing and derivative operations commute after discretization.

The variance or standard deviation (sigma) will be evaluated as pixel units if SetUseImageSpacing is off (false) or as
physical units if SetUseImageSpacing is on (true, default). The variance can be set independently in each dimension.

When the Gaussian kernel is small, this filter tends to run faster than itk::RecursiveGaussianImageFilter .\*
GaussianOperator - Image - Neighborhood - NeighborhoodOperator - RecursiveGaussianImageFilter

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| Variance                  | float64                   |                                                             |
+---------------------------+---------------------------+-------------------------------------------------------------+
| MaximumKernelWidth        | uint32                    | Set the kernel to be no wider than MaximumKernelWidth       |
|                           |                           | pixels, even if MaximumError demands it. The default is 32  |
|                           |                           | pixels.                                                     |
+---------------------------+---------------------------+-------------------------------------------------------------+
| MaximumError              | float64                   |                                                             |
+---------------------------+---------------------------+-------------------------------------------------------------+
| UseImageSpacing           | bool                      | Set/Get whether or not the filter will use the spacing of   |
|                           |                           | the input image in its calculations. Use On to take the     |
|                           |                           | image spacing information into account and to specify the   |
|                           |                           | Gaussian variance in real world units; use Off to ignore    |
|                           |                           | the image spacing and to specify the Gaussian variance in   |
|                           |                           | voxel units. Default is On.                                 |
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

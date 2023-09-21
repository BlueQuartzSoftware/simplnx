====================================
ITK Intensity Windowing Image Filter
====================================


Applies a linear transformation to the intensity levels of the input Image that are inside a user-defined interval.
Values below this interval are mapped to a constant. Values over the interval are mapped to another constant.

Group (Subgroup)
================

ITKImageIntensity (ImageIntensity)

Description
===========

IntensityWindowingImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels.
The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should
have and the lower and upper limits of the intensity window of the input image. This operation is very common in
visualization, and can also be applied as a convenient preprocessing operation for image segmentation.

All computations are performed in the precision of the input pixel’s RealType. Before assigning the computed value to
the output pixel.\* RescaleIntensityImageFilter

Parameters
==========

============= ======= ========================================================================================
Name          Type    Description
============= ======= ========================================================================================
WindowMinimum float64 Set/Get the values of the maximum and minimum intensities of the input intensity window.
WindowMaximum float64 Set/Get the values of the maximum and minimum intensities of the input intensity window.
OutputMinimum float64 Set/Get the values of the maximum and minimum intensities of the output image.
OutputMaximum float64 Set/Get the values of the maximum and minimum intensities of the output image.
============= ======= ========================================================================================

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

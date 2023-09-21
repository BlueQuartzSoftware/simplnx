==================================
ITK Rescale Intensity Image Filter
==================================


.. role:: raw-latex(raw)
   :format: latex
..

Applies a linear transformation to the intensity levels of the input Image .

Group (Subgroup)
================

ITKImageIntensity (ImageIntensity)

Description
===========

RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels.
The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should
have.

The following equation gives the mapping of the intensity values

:raw-latex:`\f[` outputPixel = ( inputPixel - inputMin)
:raw-latex:`\cdot `:raw-latex:`\frac{(outputMax - outputMin )}{(inputMax - inputMin)}` + outputMin :raw-latex:`\f]`

All computations are performed in the precision of the input pixel’s RealType. Before assigning the computed value to
the output pixel.

NOTE: In this filter the minimum and maximum values of the input image are computed internally using the
MinimumMaximumImageCalculator . Users are not supposed to set those values in this filter. If you need a filter where
you can set the minimum and maximum values of the input, please use the IntensityWindowingImageFilter . If you want a
filter that can use a user-defined linear transformation for the intensity, then please use the ShiftScaleImageFilter
.\* IntensityWindowingImageFilter

Parameters
==========

============= ======= ===========
Name          Type    Description
============= ======= ===========
OutputMinimum float64 
OutputMaximum float64 
============= ======= ===========

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

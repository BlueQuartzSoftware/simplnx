=================================
ITK Grayscale Dilate Image Filter
=================================


Grayscale dilation of an image.

Group (Subgroup)
================

ITKMathematicalMorphology (MathematicalMorphology)

Description
===========

Dilate an image using grayscale morphology. Dilation takes the maximum of all the pixels identified by the structuring
element.

The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring
element having values > 0 are candidates for affecting the center pixel.\* MorphologyImageFilter ,
GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

Parameters
==========

============ ========== ==============================================================
Name         Type       Description
============ ========== ==============================================================
KernelRadius uint32     Set the radius of the kernel structuring element.
KernelType   KernelEnum Set the kernel or structuring element used for the morphology.
============ ========== ==============================================================

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

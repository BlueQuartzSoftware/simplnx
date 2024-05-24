# Erode/Dilate Mask

## Group (Subgroup)

Processing (Cleanup)

## Description

If the mask is _dilated_, the **Filter** grows the _true_ regions by one **Cell** in an iterative sequence for a user
defined number of iterations. During the _dilate_ process, the classification of any **Cell** neighboring a _false_ *
*Cell** will be changed to _true_. If the mask is _eroded_, the **Filter** shrinks the _true_ regions by one **Cell** in
an iterative sequence for a user defined number of iterations. During the _erode_ process, the classification of the
*false* **Cells** is changed to _true_ if one of its neighbors is _true_. The **Filter** also offers the option(s) to
turn on/off the erosion or dilation in specific directions (X, Y or Z).

This filter will ONLY change the _Mask_ data array and not any of the other data arrays in the same attribute matrix.

The example images below were generated **AFTER** the execution of the filter and essentially any black pixel is where the **Mask** was false and any other color is where the **Mask** is true. (The colors are the typical IPF Colors using a <001> reference direction)

| Before Dilatation                      | After Dilation                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateMask_Before.png) | ![](Images/ErodeDilateMask_Dilate.png) |

| Before Erosion                      | After Erosion                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateMask_Before.png) | ![](Images/ErodeDilateMask_Erode.png) |

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

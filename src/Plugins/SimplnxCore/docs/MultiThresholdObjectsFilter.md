# Multi-Threshold Objects

## Group (Subgroup)

Processing (Threshold)

## Description

This **Filter** allows the user to input single or multiple criteria for thresholding **Attribute Arrays** in an **Attribute Matrix**. Internally, the algorithm creates the output boolean arrays for each comparison that the user creates.  Comparisons can be either a value and boolean operator (*Less Than*, *Greater Than*, *Equal To*, *Not Equal To*) or a collective set of comparisons. Then all the output arrays are compared with their given comparison operator ( *And* / *Or* ) with the value of a set being the result of its own comparisons calculated from top to bottom.

An example of this **Filter's** use would be after EBSD (Electron Backscatter Diffraction) data is read into DREAM3D-NX and the user wants to have DREAM3D-NX consider **Cells** that the user considers *good*. The user would insert this **Filter** and select the criteria that makes a **Cell** *good*. All arrays **must** come from the same **Attribute Matrix** in order for the **Filter** to execute.

A **Cell** is a single element (voxel) of a gridded geometry such as an **Image Geometry**, and an **Attribute Matrix** is the container that groups together all of the per-**Cell** **Attribute Arrays** (e.g., Confidence Index, Image Quality, Phases) so that they share the same number of tuples. Because the threshold is evaluated tuple-by-tuple across the selected arrays, every array used in a comparison must live in that same **Attribute Matrix**.

For example, an integer array contains the values 1, 2, 3, 4, 5. For a comparison value of 3 and the comparison operator greater than, the boolean threshold array produced will contain *false*, *false*, *false*, *true*, *true*. For the comparison set { *Greater Than* 2 AND *Less Than* 5} OR *Equals* 1, the boolean threshold array produced will contain *true*, *false*, *true*, *true*, *false*.

It is possible to set custom values for both the TRUE and FALSE values that will be output to the threshold array.  For example, if the user selects an output threshold array type of uint32, then they could set a custom FALSE value of 5 and a custom TRUE value of 20.  So then instead of outputting 0's and 1's to the threshold array, the filter would output 5's and 20's.

**NOTE**: If custom TRUE/FALSE values are chosen, then using the resulting mask array in any other filters that require a mask array will break those other filters.  This is because most other filters that require a mask array make the assumption that the true/false values are 1/0.

### Required Input Sources

- The **Attribute Arrays** being thresholded -- any per-element scalar arrays that already exist in a single **Attribute Matrix**. Common producers are EBSD readers such as [Read H5EBSD](../OrientationAnalysis/ReadH5EbsdFilter.md), [Read CTF Data](../OrientationAnalysis/ReadCtfDataFilter.md), or [Read ANG Data](../OrientationAnalysis/ReadAngDataFilter.md) (which supply Confidence Index, Image Quality, Phases, etc.), but any filter that creates scalar **Cell** or **Feature** arrays can supply the inputs.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

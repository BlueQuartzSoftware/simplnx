# Compute Kernel Average Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** determines the Kernel Average Misorientation (KAM) for each **Cell**.  The user can select the size of the kernel to be used in the calculation.  The kernel size entered by the user is the *radius* of the kernel (i.e., entering values of *1*, *2*, *3* will result in a kernel that is *3*, *5*, and *7* **Cells** in size in the X, Y and Z directions, respectively).  The algorithm for determination of KAM is as follows:

1. Calculate the misorientation angle between each **Cell** in a kernel and the central **Cell** of the kernel
2. Average all of the misorientations for the kernel and store at the central **Cell**

The **Use Feature Ids** option controls which **Cells** within the kernel are included in the average:

+ **Checked (default):** only **Cells** that belong to the same *Feature* (same *Feature Id*) as the central **Cell** are considered — the calculation will **not** cross grain boundaries. This is the traditional per-grain KAM.
+ **Unchecked:** the *Feature Id* grouping is ignored and the average may cross grain boundaries, producing a per-voxel KAM. A kernel **Cell** is still excluded if its *Feature Id* is 0 (invalid/background data) or if its *Phase* differs from the central **Cell**'s *Phase* (misorientation between different crystal structures is not defined).

In both modes, **Cells** with a *Feature Id* of 0 or a *Phase* of 0 are considered invalid and receive a KAM value of 0.

*Note:* All **Cells** in the kernel are weighted equally during the averaging, though they are not equidistant from the central **Cell**.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (04) Small IN100 Crystallographic Statistics
+ EBSD_File_Processing/aptr12_Analysis
+ EBSD_File_Processing/avtr12_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

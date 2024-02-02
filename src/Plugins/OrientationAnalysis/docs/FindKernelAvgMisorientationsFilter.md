# Find Kernel Average Misorientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines the Kernel Average Misorientation (KAM) for each **Cell**.  The user can select the size of the kernel to be used in the calculation.  The kernel size entered by the user is the *radius* of the kernel (i.e., entering values of *1*, *2*, *3* will result in a kernel that is *3*, *5*, and *7* **Cells** in size in the X, Y and Z directions, respectively).  The algorithm for determination of KAM is as follows:

1. Calculate the misorientation angle between each **Cell** in a kernel and the central **Cell** of the kernel
2. Average all of the misorientations for the kernel and store at the central **Cell**

*Note:* All **Cells** in the kernel are weighted equally during the averaging, though they are not equidistant from the central **Cell**.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ MassifPipeline
+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

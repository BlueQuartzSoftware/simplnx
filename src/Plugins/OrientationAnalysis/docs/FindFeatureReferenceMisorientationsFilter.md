# Find Feature Reference Misorientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the misorientation angle between each **Cell** within a **Feature** and a *reference orientation* for that **Feature**.  The user can choose the *reference orientation* to be used for the **Features** from a drop-down menu.  The options for the *reference orientation* are the average orientation of the **Feature** or the orientation of the **Cell** that is furthest from the *boundary* of the **Feature**.

Note: the average orientation of the **Feature** is a typical choice, but if the **Feature** has undergone plastic deformation and the amount of lattice rotation developed is of interest, then it may be more reasonable to use the orientation *near the center* of the **Feature** as it may not have rotated and thus serve as a better *reference orientation*.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

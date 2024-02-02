# Add Bad Data

## Group (Subgroup)

Synthetic Building (Misc)

## Description

This **Filter** adds "bad" data to an **Image Geometry**.  This **Filter** is intended to add "realism" (i.e., more representative of an experimental dataset) to synthetic structures that don not have any "bad" **Cells**.  The user can choose to add "random noise" and/or "noise" along **Feature** boundaries. For a given type of noise, the user must then set the volume fraction of **Cells** to set as "bad".  The volume fractions entered apply to only the set of **Cells** that the noise would affect.  For example, if the user chose *0.2* for the volume fraction of boundary "noise", then each boundary **Cell** would have a *20%* chance of being changed to a "bad" **Cell** and all other **Cells** would have a *0%* chance of being changed. In order to compute noise over the **Feature** boundaries, the **Filter** needs the Manhattan distances for each **Cell** from the **Feature** boundaries. Note that the computed Manhattan distances are floating point values, but this **Filter** requires an integer array. To create the necessary integer array, use the Convert Attributer Data Type **Filter** to cast the Manhattan distance array to an integer array.

All **Attribute Arrays** that belong to the same **Attribute Matrix** as the selected *Feature Boundary Euclidean Distances* array will have noise added to them. To flag a value as "noise", this **Filter** will initialize a selected *tuple* in the **Attribute Array** to **0**. Note that a zero value *may not* necessarily represent a "bad" data point in any kind of **Attribute Array**.

For more information on synthetic building, visit the tutorial.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

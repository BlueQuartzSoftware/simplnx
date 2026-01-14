# Compute Feature Reference Misorientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the misorientation angle between each **Cell** within a **Feature** and a 
*reference orientation* for that **Feature**.  The user can choose the *reference orientation* to 
be used for the **Features** from a drop-down menu.  The options for the *reference orientation* are 
the average orientation of the **Feature** or the orientation of the **Cell** that is furthest from 
the *boundary* of the **Feature**.

Note: the average orientation of the **Feature** is a typical choice, but if the **Feature** has 
undergone plastic deformation and the amount of lattice rotation developed is of interest, then 
it may be more reasonable to use the orientation *near the center* of the **Feature** as it may 
not have rotated and thus serve as a better *reference orientation*.

## IPF Colors <001> Direction

This is the data set's IPF colors by the <001> direction which shows the reader the relative
orientation gradients within each feature (grain).

![ComputeFeatureReferenceMisorientations_3.png](Images/ComputeFeatureReferenceMisorientations_3.png)

## Example Using Feature's Average Orientation

Using the `T12-MAI-2010/fw-ar-IF1-aptr12-corr.ctf` data set we can generate the following
data using the **Feature Average Orientation** choice.

![ComputeFeatureReferenceMisorientations_0.png](Images/ComputeFeatureReferenceMisorientations_0.png)

### Example Using Feature's Euclidean Center Voxel's Orientation

Using the same dataset, the algorithm will find the voxel that is the furthest from the
feature boundary, and use that voxel's orientation as the **reference orientation**.

![ComputeFeatureReferenceMisorientations_1.png](Images/ComputeFeatureReferenceMisorientations_1.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

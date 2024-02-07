# Neighbor Orientation Comparison (Bad Data)

## Group (Subgroup)

Orientation Analysis (Cleanup)

## Description

This **Filter** compares the orientations of *bad* **Cells** with their neighbor **Cells**.  If the misorientation is below a user defined tolerance for a user defined number of neighbor **Cells** , then the *bad* **Cell** will be changed to a *good* **Cell**.

*Note:* Only the boolean value defining the **Cell** as *good* or *bad* is changed, not the data at **Cell**.

*Note:* The **Filter** will iteratively reduce the required number of neighbors from 6 until it reaches the user defined number. So, if the user selects a required number of neighbors of 4, then the **Filter** will run with a required number of neighbors of 6, then 5, then 4 before finishing.  

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ (04) SmallIN100 Presegmentation Processing
+ INL Export
+ 04_Steiner Compact

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

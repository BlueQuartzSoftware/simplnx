# Find Feature Average Orientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines the average orientation of each **Feature** by the following algorithm:

1. Gather all **Elements** that belong to the **Feature**
2. Using the symmetry operators of the phase of the **Feature**, rotate the quaternion of the **Feature**'s first **Element** into the *Fundamental Zone* nearest to the origin
3. Rotate each subsequent **Elements**'s quaternion (with same symmetry operators) looking for the quaternion closest to the quaternion selected in Step 2
4. Average the rotated quaternions for all **Elements** and store as the average for the **Feature**

*Note:* The process of finding the nearest quaternion in Step 3 is to account for the periodicity of orientation space, which would cause problems in the averaging if all quaternions were forced to be rotated into the same *Fundamental Zone*

*Note:* The quaternions can be averaged with a simple average because the quaternion space is not distorted like Euler space.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ InsertTransformationPhase
+ (06) SmallIN100 Postsegmentation Processing
+ (05) SmallIN100 Crystallographic Statistics
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

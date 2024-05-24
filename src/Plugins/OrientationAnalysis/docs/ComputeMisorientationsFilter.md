# Compute Feature Neighbor Misorientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines, for each **Feature**, the misorientations with each of the **Features** that are in contact with it.  The misorientations are stored as a list (for each **Feature**) of angles (in degrees).  The axis of the misorientation is not stored by this **Filter**.

The user can also calculate the average misorientation between the feature and all contacting features.

### Notes

**NOTE:** Only features with identical crystal structures will be calculated. If two features have different crystal structures then a value of NaN is set for the misorientation.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

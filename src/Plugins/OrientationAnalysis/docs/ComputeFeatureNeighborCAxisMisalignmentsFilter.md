# Compute Feature Neighbor C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallographic)

## Description

For each feature, the C-Axis misalignments are determined for each neighbor of the feature. The neighbor list is a variable that is passed in by the user. This "NeighborList" could have been generated from any other appropriate filter. This means that a neighbor list could represent all neighbors that are physically connected to the current feature (Find Feature Neighbors), within a certain radius of the feature (Compute Feature Neighborhoods) or any other custom filter.

There are 2 outputs from this filter:
- The list of misalignments
- Optionally the average of all misalignments.

**The misalignment values are stored as Degrees.**

### Notes

**NOTE:** Only features with identical phase values and a crystal structure of **Hexagonal_High** will be calculated. If two features have different phase values or a crystal structure that is *not* Hexagonal_High then a value of NaN is set for the misorientation.

Results from this filter can differ from its original version in DREAM.3D 6.5.171 by around 0.0001. This version uses double precision and Eigen for matrix operations which account for the differences in output.

% Auto generated parameter table will be inserted here

## Example Pipelines

EBSD_Hexagonal_Data_Analysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

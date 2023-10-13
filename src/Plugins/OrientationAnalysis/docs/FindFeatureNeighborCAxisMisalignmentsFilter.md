# Find Feature Neighbor C-Axis Misalignments

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines, for each **Feature**, the C-axis misalignments with the **Features** that are in contact with it.  The C-axis misalignments are stored as a list (for each **Feature**) of angles (in degrees).

### Notes

**NOTE:** Only features with identical phase values and a crystal structure of **Hexagonal_High** will be calculated. If two features have different phase values or a crystal structure that is *not* Hexagonal_High then a value of NaN is set for the misorientation.

% Auto generated parameter table will be inserted here

## Example Pipelines

Combo-EBSD-osc_r0c0

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.

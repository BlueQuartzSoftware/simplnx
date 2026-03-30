# Compute Misorientation

## Group (Subgroup)

Tools (Orientations)

## Description

This filter will compute the misorientation as an Axis-Angle representation between a pair of
Euler Angles or an Euler Angle and a Reference Axis-Angle.

Use the Orientation Utility to compute an input Axis-Angle if you only have another representation.

### Computation Type

The *Computation Type* parameter provides the following choices:

- **Use Arrays [0]**: Computes the misorientation between corresponding Euler Angles in two input arrays, producing one misorientation value per tuple.
- **Use Reference Axis Angle [1]**: Computes the misorientation between each Euler Angle in a single input array and a user-supplied reference axis-angle orientation.

## Compute by Arrays

If 2 Euler Arrays are used then the output is an array with the same number of Tuples as the input
Euler Arrays and represents the misorientation between the each Euler Angle in the same index in each 
of the input Euler Angle arrays.

## Compute by Reference Orientation

In this mode the user will supply a reference orientation in the form of an Axis-Angle, where the angle portion is in degrees.
Then, for every Euler Angle in the input array, the misorientation with the reference orientation will be computed.


% Auto generated parameter table will be inserted here

## Example Pipelines


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

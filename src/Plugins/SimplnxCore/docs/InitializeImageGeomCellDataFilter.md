# Initialize Image Geometry Cell Data

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** allows the user to define a subvolume of the data set in which the **Filter** will reset all data for every **Cell** within the subvolume. The user can choose from three initialization modes: *Manual* (initialize to a user-specified value), *Random* (initialize with random values across the full range of the data type), or *Random With Range* (initialize with random values within a user-specified range).

### Initialization Type

The *Initialization Type* parameter provides the following choices:

- **Manual [0]**: Initializes every cell in the subvolume to a user-specified constant value.
- **Random [1]**: Initializes every cell in the subvolume with random values drawn from the full range of the data type.
- **Random With Range [2]**: Initializes every cell in the subvolume with random values drawn from a user-specified minimum/maximum range.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

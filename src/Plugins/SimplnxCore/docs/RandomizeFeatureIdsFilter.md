# Randomize Feature Ids

## Group (Subgroup)

Core (Filters)

## Description

***WARNING:** This filter can throw a pipeline terminating error (at runtime) if the number of tuples in the supplied Feature `Attribute Matrix` is less than the max value in the Feature Ids `DataArray`*

This filter will randomize a user selected "Feature Ids" array and update every container (`DataArray`, `NeighborList`, and `StringArray`) in the feature `Attribute Matrix`. This does not generate random data but instead uses the existing values and swaps the positions of the values in the array. The intended use case is primarily for visualization, so feature data does not appear as a smooth gradient.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

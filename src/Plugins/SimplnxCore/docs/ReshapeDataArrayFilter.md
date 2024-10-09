# Reshape Data Array

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** is used to modify the tuple shape of Data Arrays, Neighbor Lists, and String Arrays within a data structure.  It validates the new tuple dimensions to ensure they are positive and differ from the current shape, preventing unnecessary or invalid reshapes.

**NOTE:** If the input array is a Neighbor List or String Array, the filter will throw a warning if the new tuple dimensions are multi-dimensional.  This is because these array types do not support multi-dimensional tuple dimensions and the filter will default to reshaping the data to an equivalent 1-dimensional number of tuples.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

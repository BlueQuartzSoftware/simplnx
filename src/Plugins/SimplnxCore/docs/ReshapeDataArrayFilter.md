# Reshape Data Array

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** is used to modify the tuple shape of Data Arrays, Neighbor Lists, and String Arrays within a data structure.  It validates the new tuple dimensions to ensure they are positive and differ from the current shape, preventing unnecessary or invalid reshapes.

**THIS FILTER DOES NOT MOVE ANY THE VALUES IN MEMORY. IT SIMPLY UPDATES THE TUPLE DIMENSIONS. This means that if the data does not
have the proper stride for the new dimensions this could result in incorrect results.**

For example if a data set is read in from an HDF5 file with a tuple dimension of 3 x 45 this means that there are 45 columns
and 3 rows. If this data is supposed to be interpreted as 3D points, using this data within DREAM3D-NX would not
work because DREAM3D-NX is "C" ordered and the dimensions should be 3 columns x 45 rows. Using this filter will 
**NOT** result the correct ordering of the data because the filter will not move any data in memory.

**NOTE:** If the input array is a Neighbor List or String Array, the filter will throw a warning if the new tuple dimensions are multi-dimensional.  This is because these array types do not support multi-dimensional tuple dimensions and the filter will default to reshaping the data to an equivalent 1-dimensional number of tuples.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

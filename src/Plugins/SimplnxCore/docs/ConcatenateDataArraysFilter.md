# Concatenate Data Arrays

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** concatenates multiple input arrays by taking a list of input arrays and appending their data sequentially into a single output array. The concatenation process involves combining the arrays such that the order of the input arrays directly affects the structure of the output. For example, if the first input array contains 5 tuples and the second contains 7 tuples, the resulting output array will have 12 tuples, with the tuples from the second array appended directly after those from the first array.

This filter is designed to handle arrays of matching array types and component dimensions. If the arrays have different array types or component dimensions, the filter will raise an error.

**NOTE:** If you are wanting to instead combine data arrays into a multi-component array, please see the [Combine Attribute Arrays](CombineAttributeArraysFilter.md) filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

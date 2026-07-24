# Create Data Group

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** creates a new empty **DataGroup** at a user-specified location in the **Data Structure**. The new DataGroup contains nothing initially; subsequent filters can add child objects to it.

### What is a DataGroup?

A **DataGroup** is a general-purpose container. Unlike an **Attribute Matrix**, a DataGroup can hold *any* DataObject of *any size* -- geometries, attribute matrices, arrays of varying sizes, even other DataGroups. There is no shared-tuple-shape contract.

Use a DataGroup when you need to organize heterogeneous data, such as a logical grouping of unrelated arrays or geometries. Use [Create Attribute Matrix](CreateAttributeMatrixFilter.md) instead when all the arrays you are grouping share a common tuple shape and represent values over the same domain.

### Required Input Sources

- **Parent Data Object Path** -- an existing **DataGroup**, **Attribute Matrix**, or **Geometry** under which the new DataGroup will be created. The top-level of the Data Structure can also be selected.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

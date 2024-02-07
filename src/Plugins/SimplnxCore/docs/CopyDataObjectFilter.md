# Copy Data Object

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** deep copies one or more DataObjects.

**In the case of copying _DataObject_s that inherit from _BaseGroup_**, such as _DataGroup_ or _AttributeMatrix_, **it will copy all of the child objects recursively**, that is to say all of an object's children and childrens' children and so on will be copied if applicable.

Commonly used _BaseGroup_ children:

- **_ALL_** Geometries
- _DataGroup_
- _AttributeMatrix_
- _GridMontage_

See the DataStructure section of the reference manual for a complete hierarchy.

When the _Copy to New Parent_ is toggled true a new parameter will appear. This parameter, _Copied Parent Group_, allows for the selected arrays to all be copied into whatever data container you place here.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

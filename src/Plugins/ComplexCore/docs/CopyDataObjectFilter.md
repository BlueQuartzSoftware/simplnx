# Copy Data Object


## Group (Subgroup) ##

Core (Generation)

## Description ##

This **Filter** deep copies one or more DataObjects.

**In the case of copying _DataObject_s that inherit from _BaseGroup_**, such as _DataGroup_ or _AttributeMatrix_, **it will copy all of the child objects recursively**, that is to say all of an object's children and childrens' children and so on will be copied if applicable.

Commonly used _BaseGroup_ children:
- **_ALL_** Geometries
- _DataGroup_
- _AttributeMatrix_
- _GridMontage_

See the DataStructure section of the reference manual for a complete hierarchy.

When the _Copy to New Parent_ is toggled true a new parameter will appear. This parameter, _Copied Parent Group_, allows for the selected arrays to all be copied into whatever data container you place here.

## Parameters ##

None

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|----------------|
| DataObject | N/A | N/A | N/A | The list of DataObjects to copy. |
| bool | false | N/A | N/A | Whether to copy the DataObjects to a new parent or not. |
| DataGroup | N/A | N/A | N/A | The group to be used as the parent for all the DataObject copies if the Copy to New Parent option is selected |
| string | _COPY | N/A | N/A | The suffix string to be appended to each copy's name |

## Created Objects ##

A deep copy of the DataObjects selected in the input.

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: CopyDataObjectFilter
+ Displayed Name: Copy Data Object

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| existing_data_path | Objects to copy | A list of DataPaths to the DataObjects to be copied | complex.MultiPathSelectionParameter |
| new_data_path | Copied Parent Group | DataPath to parent BaseGroup in which to store the copied DataObject(s) | complex.DataGroupSelectionParameter |
| new_path_suffix | Copied Object(s) Suffix | Suffix string to be appended to each copied DataObject | complex.StringParameter |
| use_new_parent | Copy to New Parent | Copy all the DataObjects to a new BaseGroup | complex.BoolParameter |


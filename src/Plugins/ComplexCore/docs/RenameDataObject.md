# RenameDataObject

## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** renames a user chosen **Data Object** of any type.

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| New Data Object Name| String | New name for the selected **Data Object** |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Object** | None | N/A | N/A | **Data Object** to rename |

## Created Objects ##

None

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)



## Python Filter Arguments

+ module: complex
+ Class Name: RenameDataObject
+ Displayed Name: Rename DataObject

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| data_object | DataObject to Rename | DataPath pointing to the target DataObject | complex.DataPathSelectionParameter |
| new_name | New Name | Target DataObject name | complex.StringParameter |


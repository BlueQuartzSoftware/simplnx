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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: RenameDataObject
+ Displayed Name: Rename DataObject

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| data_object | DataObject to Rename | DataPath pointing to the target DataObject | complex.DataPathSelectionParameter |
| new_name | New Name | Target DataObject name | complex.StringParameter |


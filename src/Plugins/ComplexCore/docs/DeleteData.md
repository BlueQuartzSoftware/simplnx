# Delete Data 


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** allows the user to remove specified objects from the existing structure. This can be helpful if the user has operations that need as much memory as possible and there are extra objects that are not needed residing in memory. Alternatively, this **Filter** allows the user to remove objects that may share a name with another object further in the **Pipeline** that another **Filter** tries to create, since DREAM.3D generally does not allows objects at the same hierarchy to share the same name.

## Parameters ##

None

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any  | None         | Any | Any | Objects to delete |

## Created Objects ##

None

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: DeleteData
+ Displayed Name: Delete Data

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| removed_data_path | DataPaths to remove | The complete path to the DataObjects to be removed | complex.MultiPathSelectionParameter |


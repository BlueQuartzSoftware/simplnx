# Extract Component as Attribute Array


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** will do one of the following to one component of a multicomponent **Attribute Array**:
- Remove 1 component from multicomponent **Attribute Array** completely [This is done implicitly so long as **Move Extracted Components To New Array** is false]
- Extract 1 component from multicomponent **Attribute Array** and store it in a new **DataArray** without removing from original
- Extract 1 component from multicomponent **Attribute Array** and store it in a new **DataArray** and remove that component from the original

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| Move Extracted Components To New Array | bool | The bool that determines if extracted components will be stored in new array |
| Remove Extracted Components from Old Array | bool | The bool that determines if extracted components will be deleted from original array |
| Component Number to Extract | int32_t | The index of which component to extract |


## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any | >1 | Multicomponent **Attribute Array** to use as input |


## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any | (1) | Scalar **Attribute Array** name |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



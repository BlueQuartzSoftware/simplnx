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

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: complex
+ Class Name: ExtractComponentAsArrayFilter
+ Displayed Name: Extract/Remove Components

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| comp_number | Component Index to Extract | The index of the component in each tuple to be removed | complex.Int32Parameter |
| move_components_to_new_array | Move Extracted Components to New Array | If true the extracted components will be placed in a new array | complex.BoolParameter |
| new_array_path | Scalar Attribute Array | The DataArray to store the extracted components | complex.DataObjectNameParameter |
| remove_components_from_array | Remove Extracted Components from Old Array | If true the extracted components will be deleted | complex.BoolParameter |
| selected_array_path | Multicomponent Attribute Array | The array to extract componenets from | complex.ArraySelectionParameter |


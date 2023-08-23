# Create Feature Array From Element Array 


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** copies all the associated **Element** data of a selected **Element Array** to the **Feature** to which the **Elements** belong. The value stored for each **Feature** will be the value of the *last element copied*. 

## Parameters ##

None

## Required Geometry ##

Not Applicable


## Required Objects ##

| Kind  | Type | Component Dimensions | Description |
|------|------|----------------------|-------------|
| Element Array DataPath | Any | Any | **Element** data to copy to **Feature** data |
| Element Array DataPath | int32_t | (1) | Specifies to which **Feature** each **Element** belongs |
| Cell Feature **Attribute Matrix** | Cell Feauture | N/A | The path to the cell feature **Attribute Matrix** |


## Created Objects ##

| Kind | Type | Component Dimensions | Description |
|------|------|----------------------|-------------|
| Feature Array DataPath | Any  | Any | The copied array containing **Feature** data |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: CreateFeatureArrayFromElementArray
+ Displayed Name: Create Feature Array from Element Array

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_feature_attribute_matrix_path | Cell Feature Attribute Matrix | The path to the cell feature attribute matrix where the converted output feature array will be stored | complex.DataGroupSelectionParameter |
| created_array_name | Created Feature Attribute Array | The path to the copied AttributeArray | complex.DataObjectNameParameter |
| feature_ids_path | Feature Ids | Specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| selected_cell_array_path | Element Data to Copy to Feature Data | Element Data to Copy to Feature Data | complex.ArraySelectionParameter |


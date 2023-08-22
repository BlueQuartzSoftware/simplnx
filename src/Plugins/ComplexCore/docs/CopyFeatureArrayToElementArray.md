# Create Element Array from Feature Array 


## Group (Subgroup) ##

Core (Memory/Management)

## Description ##

This **Filter** copies the values associated with a **Feature** to all the **Elements** that belong to that **Feature**.  Xmdf visualization files write only the **Element** attributes, so if the user wants to display a spatial map of a **Feature** level attribute, this **Filter** will transfer that information down to the **Element** level.

## Parameters ##

None

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Feature **Attribute Array**  | None | Any | Any | **Feature** data to copy to **Element** data  |
| Element **Attribute Array** | FeatureIds  | int32_t | (1) | Specifies to which **Feature** each **Element** belongs  |


## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Element **Attribute Array** | None | Any | Any | Copied **Attribute Array**  |


## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: complex
+ Class Name: CopyFeatureArrayToElementArray
+ Displayed Name: Create Element Array from Feature Array

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| created_array_suffix | Created Array Suffix | The suffix to add to the input attribute array name when creating the copied array | complex.StringParameter |
| feature_ids_path | Feature Ids | Specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| selected_feature_array_path | Feature Data to Copy to Element Data | The DataPath to the feature data that should be copied to the cell level | complex.MultiArraySelectionParameter |


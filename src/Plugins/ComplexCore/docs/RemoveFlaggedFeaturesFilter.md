# Remove Flagged Features 


## Group (Subgroup) ##

Processing (Cleanup)

## Description ##

This **Filter** will remove **Features** that have been flagged by another **Filter** from the structure.  The **Filter** requires that the user point to a boolean array at the **Feature** level that tells the **Filter** whether the **Feature** should remain in the structure.  If the boolean array is *false* for a **Feature**, then all **Cells** that belong to that **Feature** are temporarily *unassigned* and after all *undesired* **Features** are removed, the remaining **Features** are isotropically coarsened to fill in the gaps left by the removed **Features**.

## Parameters ##

None

## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| **Feature Attribute Array** | Active | bool | (1) | Specifies whether the **Feature** will remain in the structure or not |

## Created Objects ##

None

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)



## Python Filter Arguments

+ module: complex
+ Class Name: RemoveFlaggedFeaturesFilter
+ Displayed Name: Extract/Remove Flagged Features

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| created_image_geometry_prefix | Created Image Geometry Prefix | The prefix name for each of new cropped (extracted) geometry 

NOTE: a '-' will automatically be added between the prefix and number | complex.StringParameter |
| feature_ids_path | Cell Feature Ids | Specifies to which Feature each cell belongs | complex.ArraySelectionParameter |
| fill_removed_features | Fill-in Removed Features | Whether or not to fill in the gaps left by the removed Features | complex.BoolParameter |
| flagged_features_array_path | Flagged Features | Specifies whether the Feature will remain in the structure or not | complex.ArraySelectionParameter |
| functionality | Selected Operation | Whether to extract features into new geometry or remove or extract then remove | complex.ChoicesParameter |
| ignored_data_array_paths | Attribute Arrays to Ignore | The list of arrays to ignore when removing flagged features | complex.MultiArraySelectionParameter |
| image_geometry | Selected Image Geometry | The target geometry | complex.GeometrySelectionParameter |


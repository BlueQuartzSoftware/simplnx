# Resample Image Geometry

## Group (Subgroup) ##

Sampling (Resample)

## Description ##

This **Filter** changes the **Cell** spacing/resolution based on inputs from the user. The values entered are the desired new spacings (not multiples of the current resolution).  The number of **Cells** in the volume will change when the spacing values are changed and thus the user should be cautious of generating "too many" **Cells** by entering very small values (i.e., very high resolution). Thus, this **Filter** will perform a down-sampling or up-sampling procedure.  

A new grid of **Cells** is created and "overlaid" on the existing grid of **Cells**.  There is currently no *interpolation* performed, rather the attributes of the old **Cell** that is closest to each new **Cell's** is assigned to that new **Cell**. 

*Note:* Present **Features** may disappear when down-sampling to coarse resolutions. If *Renumber Features* is checked, the **Filter** will check if this is the case and resize the corresponding **Feature Attribute Matrix** to comply with any changes. Additionally, the **Filter** will renumber **Features** such that they remain contiguous. 

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Sapcing | float (3x) | The new resolution values (dx, dy, dz) |
| Renumber Features | bool | Whether the **Features** should be renumbered |
| Remove Original Geometry | bool | Whether the current **Geometry** should be removed after the resampling |

## Required Geometry ##

Image 

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | CellData | Cell | N/A | **Cell Attribute Matrix** that holds data for resolution change |
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs. Only required if *Renumber Features* is checked |
| **Attribute Matrix** | CellFeatureData | Cell Feature | N/A | **Feature Attribute Matrix** that corresponds to the **Feature** data for the selected _Feature Ids_. Only required if *Renumber Features* is checked |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | NewImageDataContainer | N/A | N/A | Created **Data Container** name with an **Image Geometry**. |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ResampleImageGeomFilter
+ Displayed Name: Resample Data (Image Geometry)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_feature_attribute_matrix_path | Cell Feature Attribute Matrix | DataPath to the feature Attribute Matrix | complex.AttributeMatrixSelectionParameter |
| feature_ids_path | Feature IDs | DataPath to Cell Feature IDs array | complex.ArraySelectionParameter |
| new_data_container_path | Created Image Geometry | The location of the resampled geometry | complex.DataGroupCreationParameter |
| remove_original_geometry | Perform In Place | Removes the original Image Geometry after filter is completed | complex.BoolParameter |
| renumber_features | Renumber Features | Specifies if the feature IDs should be renumbered | complex.BoolParameter |
| selected_image_geometry | Selected Image Geometry | The target geometry to resample | complex.GeometrySelectionParameter |
| spacing | New Spacing | The new spacing values (dx, dy, dz). Larger spacing will cause less voxels, smaller spacing will cause more voxels. | complex.VectorFloat32Parameter |


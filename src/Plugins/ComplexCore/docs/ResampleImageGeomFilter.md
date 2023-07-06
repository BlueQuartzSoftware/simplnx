# Resample Image Geometry

## Group (Subgroup) ##

Sampling (Resample)

## Description ##

This **Filter** changes the **Cell** spacing/resolution based on inputs from the user. There are several resampling modes:

### Spacing ###
The values entered are the desired new spacings (not multiples of the current resolution).  The number of **Cells** in the volume will change when the spacing values are changed and thus the user should be cautious of generating "too many" **Cells** by entering very small values (i.e., very high resolution).

### Scale Factor ###
The values entered are the desired scaling factor for each dimension, in percentages.  (50%, 50%, 50%) would resample the geometry by half in all dimensions.

### Exact Dimensions ###
The values entered are the desired cell dimensions of the resampled geometry.  (100, 100, 100) would resample the geometry so that there are 100 cells in each dimension.

---

A new grid of **Cells** is created and "overlaid" on the existing grid of **Cells**.  There is currently no *interpolation* performed, rather the attributes of the old **Cell** that is closest to each new **Cell's** is assigned to that new **Cell**. 

*Note:* Present **Features** may disappear when down-sampling to coarse resolutions. If *Renumber Features* is checked, the **Filter** will check if this is the case and resize the corresponding **Feature Attribute Matrix** to comply with any changes. Additionally, the **Filter** will renumber **Features** such that they remain contiguous. 

*Note:* This filter does NOT change the overall bounds of the volume, just the spacing and number of cells in the volume.  To change the overall bounds of the volume, apply a scaling transformation with the [Apply Transformation To Geometry](./ApplyTransformationToGeometryFilter.md) filter.

## Parameters ##

| Name                     | Type        | Description                                                                                                 |
|--------------------------|-------------|-------------------------------------------------------------------------------------------------------------|
| Resampling Mode          | int         | The resampling mode used to resample the image geometry.  Choices are Spacing, Scaling, or Exact Dimensions |
| Spacing                  | float (3x)  | The new resolution values (dx, dy, dz)                                                                      |
| Scale Factor             | float (3x)  | The scale factor (dx, dy, dz), in percentages.                                                              |
| Exact Dimensions         | uint64 (3x) | The exact dimensions (dx, dy, dz) to resample the geometry.                                                 |
| Renumber Features        | bool        | Whether the **Features** should be renumbered                                                               |
| Remove Original Geometry | bool        | Whether the current **Geometry** should be removed after the resampling                                     |

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



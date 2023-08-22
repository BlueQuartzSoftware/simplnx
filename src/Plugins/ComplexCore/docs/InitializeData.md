# Initialize Data 


## Group (Subgroup) ##

Processing (Cleanup)

## Description ##

This **Filter** allows the user to define a subvolume of the data set in which the **Filter** will reset all data by writing *zeros (0)* into every array for every **Cell** within the subvolume.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| X Min | int32_t | Minimum X bound in **Cells** |
| Y Min | int32_t | Minimum Y bound in **Cells** |
| Z Min | int32_t | Minimum Z bound in **Cells** |
| X Max | int32_t | Maximum X bound in **Cells** |
| Y Max | int32_t | Maximum Y bound in **Cells** |
| Z Max | int32_t | Maximum Z bound in **Cells** |

## Required Geometry ##

Image 

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | CellData | Cell | N/A | **Cell** data in which to initialize a subvolume to zeros |

## Created Objects ##

None

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: InitializeData
+ Displayed Name: Initialize Data

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_arrays | Cell Arrays | The cell data arrays in which to initialize a sub-volume to zeros | complex.MultiArraySelectionParameter |
| image_geom_path | Image Geometry | The geometry containing the cell data for which to initialize | complex.GeometrySelectionParameter |
| init_range | Initialization Range | The initialization range if Random With Range Initialization Type is selected | complex.VectorFloat64Parameter |
| init_type | Initialization Type | Tells how to initialize the data | complex.ChoicesParameter |
| init_value | Initialization Value | The initialization value if Manual Initialization Type is selected | complex.Float64Parameter |
| max_point | Max Point | The maximum x, y, z bound in cells | complex.VectorUInt64Parameter |
| min_point | Min Point | The minimum x, y, z bound in cells | complex.VectorUInt64Parameter |


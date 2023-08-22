# Fuse Regular Grids (Nearest Points)

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** fuses two **Image Geometry** data sets together. The grid of **Cells** in the *Reference* **Data Container** is overlaid on the grid of **Cells** in the *Sampling* **Data Container**.  Each **Cell** in the *Reference* **Data Container** is associated with the nearest point in the *Sampling* **Data Container** (i.e., no *interpolation* is performed).  All the attributes of the **Cell** in the *Sampling* **Data Container** are then assigned to the **Cell** in the *Reference* **Data Container**.  Additional to the **Cell** attributes being copied, all **Feature and Ensemble Attribute Matrices** from the *Sampling* **Data Container** are copied to the *Reference* **Data Container**.

*Note:* The *Sampling* **Data Container** remains identical after this **Filter**, but the *Reference* **Data Container**, while "geometrically identical", gains all the attribute arrays from the *Sampling* **Data Container**.

## Parameters

None

## Required Geometry

Image

## Required Objects

None

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | CellData | Cell | N/A | *Reference* **Cell** data to use for fusion |
| **Attribute Matrix** | CellData | Cell | N/A | *Sampling* **Cell** data to use for fusion |

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: NearestPointFuseRegularGridsFilter
+ Displayed Name: Fuse Regular Grids (Nearest Point)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| fill_value | Fill Value | This is the value that will appear in the arrays outside the overlap | complex.Float64Parameter |
| reference_cell_attribute_matrix_path | Reference Cell Attribute Matrix | The attribute matrix for the reference geometry | complex.DataGroupSelectionParameter |
| reference_geometry_path | Reference Image Geometry | This is the geometry that will store the values from the overlap | complex.GeometrySelectionParameter |
| sampling_cell_attribute_matrix_path | Sampling Cell Attribute Matrix | The attribute matrix for the sampling geometry | complex.DataGroupSelectionParameter |
| sampling_geometry_path | Sampling Image Geometry | This is the geometry that will be copied into the reference geometry at the overlap | complex.GeometrySelectionParameter |
| use_fill | Use Custom Fill Value | If false all copied arrays will be filled with 0 by default | complex.BoolParameter |


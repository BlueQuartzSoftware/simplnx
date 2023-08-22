# Neighbor Orientation Comparison (Bad Data)  #


## Group (Subgroup) ##

Orientation Analysis (Cleanup)

## Description ##

This **Filter** compares the orientations of *bad* **Cells** with their neighbor **Cells**.  If the misorientation is below a user defined tolerance for a user defined number of neighbor **Cells** , then the *bad* **Cell** will be changed to a *good* **Cell**.

*Note:* Only the boolean value defining the **Cell** as *good* or *bad* is changed, not the data at **Cell**.

*Note:* The **Filter** will iteratively reduce the required number of neighbors from 6 until it reaches the user defined number. So, if the user selects a required number of neighbors of 4, then the **Filter** will run with a required number of neighbors of 6, then 5, then 4 before finishing.  

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Misorientation Tolerance (Degrees) | float | Angular tolerance used to compare with neighboring **Cells** |
| Required Number of Neighbors | int32_t | Minimum number of neighbor **Cells** that must have orientations within above tolerace to allow **Cell** to be changed |

## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | Quaternions | flaot | (4) | Specifies the orientation of the **Cell** in quaternion representation |
| **Cell Attribute Array** | Mask | bool | (1) | Used to define **Cells** as *good* or *bad*  |
| **Cell Attribute Array** | Cell Phases | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Ensemble Attribute Array** | Crystal Structures | uint32_t | (1) | Enumeration representing the crystal structure for each phase |

## Created Objects ##

None

## Example Pipelines ##

+ (10) SmallIN100 Full Reconstruction
+ (04) SmallIN100 Presegmentation Processing
+ INL Export
+ 04_Steiner Compact

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: BadDataNeighborOrientationCheckFilter
+ Displayed Name: Neighbor Orientation Comparison (Bad Data)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_phases_array_path | Cell Phases | Specifies to which Ensemble each Cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each phase | complex.ArraySelectionParameter |
| good_voxels_array_path | Mask | Used to define Cells as good or bad | complex.ArraySelectionParameter |
| image_geometry_path | Image Geometry | The target geometry | complex.GeometrySelectionParameter |
| misorientation_tolerance | Misorientation Tolerance (Degrees) | Angular tolerance used to compare with neighboring Cells | complex.Float32Parameter |
| number_of_neighbors | Required Number of Neighbors | Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed | complex.Int32Parameter |
| quats_array_path | Quaternions | Specifies the orientation of the Cell in quaternion representation | complex.ArraySelectionParameter |


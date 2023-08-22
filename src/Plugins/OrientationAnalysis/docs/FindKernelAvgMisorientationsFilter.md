# Find Kernel Average Misorientations

## Group (Subgroup) ##

Statistics (Crystallographic)

## Description ##

This **Filter** determines the Kernel Average Misorientation (KAM) for each **Cell**.  The user can select the size of the kernel to be used in the calculation.  The kernel size entered by the user is the *radius* of the kernel (i.e., entering values of *1*, *2*, *3* will result in a kernel that is *3*, *5*, and *7* **Cells** in size in the X, Y and Z directions, respectively).  The algorithm for determination of KAM is as follows:

1. Calculate the misorientation angle between each **Cell** in a kernel and the central **Cell** of the kernel
2. Average all of the misorientations for the kernel and store at the central **Cell**

*Note:* All **Cells** in the kernel are weighted equally during the averaging, though they are not equidistant from the central **Cell**.

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| Kernel Radius | int32_t (3x) | Size of the kernel in the X, Y and Z directions (in number of **Cells**) |

## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| **Cell Attribute Array**     | Phases            | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Cell Attribute Array** | Quats | float | (4) | Specifies the orientation of the **Cell** in quaternion representation |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | KernelAverageMisorientations | float | (1) | Average misorientation (in Degrees) for all **Cells** within the kernel and the central **Cell** |

## Example Pipelines ##

+ MassifPipeline
+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindKernelAvgMisorientationsFilter
+ Displayed Name: Find Kernel Average Misorientations

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_phases_array_path | Cell Phases | Specifies to which Ensemble each Cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| feature_ids_path | Cell Feature Ids | Specifies to which Feature each cell belongs | complex.ArraySelectionParameter |
| kernel_average_misorientations_array_name | Kernel Average Misorientations | The name of the array containing the average  misorientation (in Degrees) for all Cells within the kernel and the central Cell | complex.DataObjectNameParameter |
| kernel_size | Kernel Radius | Size of the kernel in the X, Y and Z directions (in number of Cells) | complex.VectorInt32Parameter |
| quats_array_path | Quaternions | Specifies the orientation of the Cell in quaternion representation | complex.ArraySelectionParameter |
| selected_image_geometry_path | Selected Image Geometry | Path to the target geometry | complex.GeometrySelectionParameter |


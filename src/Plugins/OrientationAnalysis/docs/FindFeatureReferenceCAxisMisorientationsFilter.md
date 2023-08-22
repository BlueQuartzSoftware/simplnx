# Find Feature Reference C-Axis Misalignments 


## Group (Subgroup) 

Statistics (Crystallographic)

## Description 

This **Filter** calculates the misorientation angle between the C-axis of each **Cell** within a **Feature** and the average C-axis for that **Feature** and stores that value for each **Cell**.  The average and standard deviation of those values for all **Cells** belonging to the same **Feature** is also stored for each **Feature**.

This filter requires at least one Hexagonal crystal structure phase (Hexagonal-Low 6/m or Hexagonal-High 6/mmm). Although it is not recommended, you can give input data with mixed phase types and all non hexagonal phases will be skipped in the calculations.

## Parameters 

None

## Required Geometry 

Image

## Required Objects 

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| **Cell Attribute Array** | Quats | float | (4) | Specifies the orientation of the **Cell** in quaternion representation |
| **Cell Attribute Array**     | Phases            | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Feature Attribute Array** | AvgCAxes | float | (3) | The direction <u,v,w> of the **Feature's** C-axis in the sample reference frame |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects 

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureReferenceCAxisMisorientations | float | (1) | Misorientation angle (in degrees) between **Cell's** C-axis and the C-axis of the **Feature** that owns that **Cell** |
| **Feature Attribute Array** | FeatureAvgCAxisMisorientations | float | (1) | Average of the *FeatureReferenceCAxisMisorientation* values for all of the **Cells** that belong to the **Feature** |
| **Feature Attribute Array** | FeatureStdevCAxisMisorientations | float | (1) | Standard deviation of the *FeatureReferenceCAxisMisorientation* values for all of the **Cells** that belong to the **Feature** |


## Example Pipelines 

Combo-EBSD-osc_r0c0

## License & Copyright 

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists 

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindFeatureReferenceCAxisMisorientationsFilter
+ Displayed Name: Find Feature Reference C-Axis Misalignments

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| avg_c_axes_array_path | Average C-Axes | The direction of the Feature's C-axis in the sample reference frame | complex.ArraySelectionParameter |
| cell_phases_array_path | Phases | Specifies to which Ensemble each Cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| feature_avg_c_axis_misorientations_array_name | Average C-Axis Misorientations | Average of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature | complex.DataObjectNameParameter |
| feature_ids_array_path | Feature Ids | Data Array that specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| feature_reference_c_axis_misorientations_array_name | Feature Reference C-Axis Misorientations | Misorientation angle (in degrees) between Cell's C-axis and the C-axis of the Feature that owns that Cell | complex.DataObjectNameParameter |
| feature_stdev_c_axis_misorientations_array_name | Feature Stdev C-Axis Misorientations | Standard deviation of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature | complex.DataObjectNameParameter |
| image_geometry_path | Image Geometry | The path to the input image geometry | complex.GeometrySelectionParameter |
| quats_array_path | Quaternions | Specifies the orientation of the Cell in quaternion representation | complex.ArraySelectionParameter |


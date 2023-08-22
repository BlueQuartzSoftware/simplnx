# Find Average C-Axis Orientations 


## Group (Subgroup) 

Statistics (Crystallographic)

## Description 

This **Filter** determines the average C-axis location of each **Feature** by the following algorithm:

1. Gather all **Elements** that belong to the **Feature**
2. Determine the location of the c-axis in the sample *reference frame* for the rotated quaternions for all **Elements**
3.  Average the locations and store as the average for the **Feature**

*Note:* This **Filter** will only work properly for *Hexagonal* materials.  The **Filter** does not apply any symmetry operators because there is only one c-axis (<001>) in *Hexagonal* materials and thus all symmetry operators will leave the c-axis in the same position in the sample *reference frame*.  However, in *Cubic* materials, for example, the {100} family of directions are all equivalent and the <001> direction will change location in the sample *reference frame* when symmetry operators are applied. 

## Parameters 

None

## Required Geometry 

Not Applicable

## Required Objects 

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Attribute Array**  | Quats | float | (4) | Specifies the orientation of the **Element** in quaternion representation |
| **Element Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Element** belongs. |
| **Cell Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects 

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | AvgCAxes | float | (3) | The direction <u,v,w> of the **Feature's** C-axis in the sample reference frame |


## Example Pipelines 

Combo-EBSD-osc_r0c0

## License & Copyright 

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists 

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindAvgCAxesFilter
+ Displayed Name: Find Average C-Axis Orientations

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| avg_c_axes_array_path | Average C-Axes | The output average C-Axis values for each feature | complex.DataObjectNameParameter |
| cell_feature_attribute_matrix | Cell Feature Attribute Matrix | The path to the cell feature attribute matrix | complex.AttributeMatrixSelectionParameter |
| cell_phases_array_path | Phases | Specifies to which Ensemble each Cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| feature_ids_array_path | Feature Ids | Data Array that specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| quats_array_path | Quaternions | Input quaternion array | complex.ArraySelectionParameter |


# Find C-Axis Locations


## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines the direction <u,v,w> of the C-axis for each **Element** by applying the quaternion of the **Element** to the <001> direction, which is the C-axis for *Hexagonal* materials.  This will tell where the C-axis of the **Element** sits in the *sample reference frame*.

*Note:* This **Filter** will only work properly for *Hexagonal* materials.  The **Filter** does not apply any symmetry operators because there is only one c-axis (<001>) in *Hexagonal* materials and thus all symmetry operators will leave the c-axis in the same position in the sample *reference frame*.  However, in *Cubic* materials, for example, the {100} family of directions are all equivalent and the <001> direction will change location in the *sample reference frame* when symmetry operators are applied. 

## Parameters

None

## Required Geometry

Not Applicable

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Attribute Array** | Quats | float | (4) | Specifies the orientation of the **Element** in quaternion representation |
| **Cell Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Attribute Array** | CAxisLocation | float | (3) | Direction <u,v,w> of the C-axis for each **Element** in the sample reference frame |


## Example Pipelines

Combo-EBSD-osc_r0c0

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindCAxisLocationsFilter
+ Displayed Name: Find C-Axis Locations

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| c_axis_locations_array_name | C-Axis Locations | DataPath to calculated C-Axis locations | complex.DataObjectNameParameter |
| cell_phases_array_path | Phases | Specifies to which Ensemble each Cell belongs | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| quats_array_path | Quaternions | DataPath to input quaternion values | complex.ArraySelectionParameter |


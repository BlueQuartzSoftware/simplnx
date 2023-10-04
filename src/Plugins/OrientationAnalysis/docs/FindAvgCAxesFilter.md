# Find Average C-Axis Orientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines the average C-axis location of each **Feature** by the following algorithm:

1. Gather all **Elements** that belong to the **Feature**
2. Determine the location of the c-axis in the sample *reference frame* for the rotated quaternions for all **Elements**
3. Average the locations and store as the average for the **Feature**

*Note:* This **Filter** will only work properly for *Hexagonal* materials.  The **Filter** does not apply any symmetry operators because there is only one c-axis (<001>) in *Hexagonal* materials and thus all symmetry operators will leave the c-axis in the same position in the sample *reference frame*.  However, in *Cubic* materials, for example, the {100} family of directions are all equivalent and the <001> direction will change location in the sample *reference frame* when symmetry operators are applied.

## Parameters

None

## Required Geometry

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Element Attribute Array**  | Quats | float | (4) | Specifies the orientation of the **Element** in quaternion representation |
| Element Attribute Array | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Element** belongs. |
| Cell Attribute Array | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| Ensemble Attribute Array | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each Ensemble |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | AvgCAxes | float | (3) | The direction <u,v,w> of the **Feature's** C-axis in the sample reference frame |

## Example Pipelines

Combo-EBSD-osc_r0c0

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

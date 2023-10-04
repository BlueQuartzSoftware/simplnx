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

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Cell Attribute Array | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| Cell Attribute Array | Quats | float | (4) | Specifies the orientation of the **Cell** in quaternion representation |
| Cell Attribute Array**     | Phases            | int32_t | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| Feature Attribute Array | AvgCAxes | float | (3) | The direction <u,v,w> of the **Feature's** C-axis in the sample reference frame |
| Ensemble Attribute Array | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each Ensemble |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Cell Attribute Array | FeatureReferenceCAxisMisorientations | float | (1) | Misorientation angle (in degrees) between **Cell's** C-axis and the C-axis of the **Feature** that owns that **Cell |
| Feature Attribute Array | FeatureAvgCAxisMisorientations | float | (1) | Average of the *FeatureReferenceCAxisMisorientation* values for all of the **Cells** that belong to the **Feature |
| Feature Attribute Array | FeatureStdevCAxisMisorientations | float | (1) | Standard deviation of the *FeatureReferenceCAxisMisorientation* values for all of the **Cells** that belong to the **Feature |

## Example Pipelines

Combo-EBSD-osc_r0c0

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

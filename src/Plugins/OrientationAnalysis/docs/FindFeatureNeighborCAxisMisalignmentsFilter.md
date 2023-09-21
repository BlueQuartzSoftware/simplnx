# Find Feature Neighbor C-Axis Misalignments 


## Group (Subgroup) 

Statistics (Crystallographic)

## Description

This **Filter** determines, for each **Feature**, the C-axis misalignments with the **Features** that are in contact with it.  The C-axis misalignments are stored as a list (for each **Feature**) of angles (in degrees).

### Notes 

__NOTE:__ Only features with identical phase values and a crystal structure of **Hexagonal_High** will be calculated. If two features have different phase values or a crystal structure that is *not* Hexagonal_High then a value of NaN is set for the misorientation.

## Parameters

| Name | Type | Description |
|------|------| ----------- |
|Find Average Misalignment Per Feature | bool | Whether the *average* of the C-axis misalignments with the neighboring **Features** should be stored for each **Feature |

## Required Geometry

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Feature Attribute Array | NeighborLists | List of int32_t | (1) | List of the contiguous neighboring **Features** for a given **Feature |
| Feature Attribute Array | AvgQuats | float | (4) | Defines the average orientation of the **Feature** in quaternion representation |
| Feature Attribute Array | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| Ensemble Attribute Array | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble |

## Created Objects 

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Feature Attribute Array | CAxisMisalignmentList | List of float | (1) | List of the C-axis misalignment angles (in degrees) with the contiguous neighboring **Features** for a given **Feature |
| Feature Attribute Array | AvgCAxisMisalignments | float | (1) | Number weighted average of neighbor C-axis misalignments. Only created if *Find Average Misalignment Per Feature* is checked |


## Example Pipelines 

Combo-EBSD-osc_r0c0

## License & Copyright 

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



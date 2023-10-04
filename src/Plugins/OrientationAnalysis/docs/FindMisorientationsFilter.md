# Find Feature Neighbor Misorientations

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** determines, for each **Feature**, the misorientations with each of the **Features** that are in contact with it.  The misorientations are stored as a list (for each **Feature**) of angles (in degrees).  The axis of the misorientation is not stored by this **Filter**.

The user can also calculate the average misorientation between the feature and all contacting features.

### Notes

**NOTE:** Only features with identical crystal structures will be calculated. If two features have different crystal structures then a value of NaN is set for the misorientation.

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| Find Average Misorientation Per Feature | bool | Specifies if the *average* of the misorienations with the neighboring **Features** should be stored for each **Feature |

## Required Geometry

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | NeighborLists | List of int32_t | (1) | List of the contiguous neighboring **Features** for a given **Feature |
| Feature Attribute Array | AvgQuats | float | (4) | Defines the average orientation of the **Feature** in quaternion representation |
| Feature Attribute Array | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| Ensemble Attribute Array | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each Ensemble |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | MisorientationLists | List of float | (1) | List of the misorientation angles with the contiguous neighboring **Features** for a given **Feature |
| Feature Attribute Array | AvgMisorientation | float | (1) | Number weighted average of neighbor misorientations. Only created if *Find Average Misorientation Per Feature* is checked |

## Example Pipelines

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

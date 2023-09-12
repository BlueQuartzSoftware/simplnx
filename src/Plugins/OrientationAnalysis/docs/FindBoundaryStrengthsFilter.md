# Find Feature Boundary Strength Metrics


## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates the same metrics as in the Find Neighbor Slip Transmission Metrics **Filter**.  However, this **Filter** stores the values in the **Face Attribute Matrix** of a **Triangle Geometry**.  The algorithm the **Filter** uses is as follows:

1. Find the two **Features** that are separated by a **Face** in the **Triangle Geometry**
2. Get the average orientation of both of the **Features**
3. Calculate the transmission metrics from both directions (i.e. **Feature** 1 to **Feature** 2 and **Feature** 2 to **Feature** 1)
4. Store the metrics in the **Face** map
5. Repeat for all **Faces**

*Note:* Each metric is calculated twice for the two different directions slip could approach the boundary.  The values are stored on each **Face** in the **Face** map in a way that notes the direction (i.e., when **Feature** 1 has neighbor **Feature** 2 and when **Feature** 2 has neighbor **Feature** 1) because the direction across the boundary between the **Features** affects the value of the metric.

## Parameters

| Name | Type | Description |
|------|------| ----------- |
| Loading Direction | float32 (3x) | The loading axis for the sample |

## Required Geometry

Image + Triangle

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Attribute Array** | FaceLabels | int32 | (2) | Specifies which **Features** are on either side of each **Face** |
| **Feature Attribute Array** | AvgQuats | float32 | (4) | Specifies the average orientation of each **Feature** in quaternion representation |
| **Feature Attribute Array** | Phases | int32 | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Ensemble Attribute Array** | CrystalStructures | uint32 | (1) | Enumeration representing the crystal structure for each phase |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Attribute Array** | F1s | float32 | (2) | |
| **Face Attribute Array** | F1spts | float32 | (2) | |
| **Face Attribute Array** | F7s | float32 | (2) | |
| **Face Attribute Array** | mPrimes | float32 | (2) | |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



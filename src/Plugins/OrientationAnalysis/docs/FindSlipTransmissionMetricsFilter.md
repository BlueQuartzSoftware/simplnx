# Find Neighbor Slip Transmission Metrics

## Group (Subgroup)

Statistics (Crystallographic)

## Description

This **Filter** calculates a suite of *slip transmission metrics* that are related to the alignment of slip directions and planes across **Feature** boundaries.  The algorithm for calculation of these metrics is as follows:

1. Get the average orientation of the **Feature**
2. Get the **Feature**'s list of neighboring **Features**
3. Get the average orientation of each neighboring **Feature**
4. Calculate metrics given by equations in *slip transmission metrics*
5. Store metrics in lists for the **Feature**
6. Repeat for all **Features**

*Note:* The transmission metrics are calculated using the average orientations for neighboring **Features** and not the local orientation near the boundary. Also, the metrics are calculated twice (i.e., when **Feature** 1 has neighbor **Feature** 2 and when **Feature** 2 has neighbor **Feature** 1) because the direction across the boundary between the **Features** affects the value of the metric.
  
## Parameters

None

## Required Geometry

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | NeighborList | List of int32 | (1) | List of the contiguous neighboring **Features** for a given **Feature |
| Feature Attribute Array | AvgQuats | float32 | (4) | Specifies the average orienation of each **Feature** in quaternion representation |
| Feature Attribute Array | Phases | int32 | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| Ensemble Attribute Array | CrystalStructures | uint32 | (1) | Enumeration representing the crystal structure for each Ensemble |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | F1s | float32 | (2) | |
| Feature Attribute Array | F1spts | float32 | (2) | |
| Feature Attribute Array | F7s | float32 | (2) | |
| Feature Attribute Array | mPrimes | float32 | (2) | |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

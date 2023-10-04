# Find Feature Phases

## Group (Subgroup)

Generic (Misc)

## Description

This **Filter** determines the **Ensemble** of each **Feature** by querying the **Ensemble** of the **Elements** that belong to the **Feature**. Note that it is assumed that all **Elements** belonging to a **Feature** are of the same **Feature**, and thus any **Element** can be used to determine the **Ensemble** of the **Feature** that owns that **Element**.

## Parameters

None

## Required Geometry

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Element Attribute Array | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Element** belongs |
| Element Attribute Array | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Element** belongs |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Feature Attribute Array | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs  |

## Example Pipelines

+ (10) SmallIN100 Full Reconstruction
+ INL Export
+ (06) SmallIN100 Postsegmentation Processing

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

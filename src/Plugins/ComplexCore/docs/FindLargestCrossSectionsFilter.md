# Find Feature Largest Cross-Section Areas


## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the largest cross-sectional area on a user-defined plane for all **Features**.  The **Filter** simply iterates through all **Cells** (on each section) asking for **Feature** that owns them.  On each section, the count of **Cells** for each **Feature** is then converted to an area and stored as the *LargestCrossSection* if the area for the current section is larger than the existing *LargestCrossSection* for that **Feature**.

## Parameters

| Name | Type | Description |
|------|------| ----------- |
| Plane of Interest | Enumeration | Specifies which plane to consider when determining the maximum cross-section for each **Feature** |

## Required Geometry

Image 

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | LargestCrossSection | flaot | (1) | Area of largest cross-section for **Feature** perpendicular to the user specified direction |


## Example Pipelines



## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



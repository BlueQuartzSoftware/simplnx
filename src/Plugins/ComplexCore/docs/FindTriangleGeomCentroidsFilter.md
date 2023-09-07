# Find Feature Centroids from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines the centroids of each **Feature** in a **Triangle Geometry**. The centroids are determined
using the following algorithm:

1. Query each triangle within the **Triangle Geometry** to determine its two owners
2. Add the 3 nodes of that triangle to the set of nodes bounding those two owners (*Note that a set will only allow each
   node to be entered once for a given owner*)
3. For each **Feature**, find the average (x,y,z) coordinate from the set of nodes that bound it

## Parameters

None

## Required Geometry

Triangle

## Required Objects

| Kind                     | Default Name    | Type         | Component Dimensions | Description                                                      |
|--------------------------|-----------------|--------------|----------------------|------------------------------------------------------------------|
| **Face Attribute Array** | FaceLabels      | int32_t      | (2)                  | Specifies which **Features** are on either side of each **Face** |
| **Attribute Matrix**     | FaceFeatureData | Face Feature | N/A                  | **Feature Attribute Matrix** of the selected *Face Labels*       |

## Created Objects

| Kind                        | Default Name | Type  | Component Dimensions | Description                                                        |
|-----------------------------|--------------|-------|----------------------|--------------------------------------------------------------------|
| **Feature Attribute Array** | Centroids    | float | (3)                  | Coordinates of the center of mass for a given enclosed **Feature** |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



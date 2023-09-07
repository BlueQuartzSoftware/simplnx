# Find Surface Features

## Group (Subgroup)

Generic (Spatial)

## Description

This **Filter** determines whether a **Feature** touches an outer surface of the sample. This is accomplished by simply querying the **Feature** owners of the **Cells** that sit at either . Any **Feature** that owns one of those **Cells** is said to touch an outer surface and all other **Features** are said to not touch an outer surface of the sample.

This **Filter** determines whether a **Feature** touches an outer *Surface* of the sample volume. A **Feature** is considered touching the *Surface* of the sample if either of the following conditions are met:

+ Any cell location is x<sub>min</sub>, x<sub>max</sub>, y<sub>min</sub>, y<sub>max</sub>, z<sub>min</sub> or z<sub>max</sub>
+ Any cell has **Feature ID = 0** as a neighbor.

The output of this filter is a **Feature** level array of booleans where 0=Interior/Not touching and 1=Surface/Touching.

_Note_: If there are voxels within the volume that have **Feature ID=0** then any feature touching those voxels will be considered a *Surface* feature.

_Note_: The version of this filter in legacy DREAM.3D had two bugs: one that indexed into neighboring features incorrectly (DREAM3D repo issue #988), and another that incorrectly labeled feature 0 as a surface feature when feature 0 exists in the feature ids array (DREAM3D repo issue #989). Both of these bugs have been fixed in this new version.

### 2D Image Geometry

If the structure/data is actually 2D, then the dimension that is planar is not considered and only the **Features** touching the edges are considered surface **Features**.

### Example Output

|  |   |
|-------|--------|
| ![FindSurfaceFeatures_Cylinder](Images/FindSurfaceFeatures_Cylinder.png) |  ![FindSurfaceFeatures_Square](Images/FindSurfaceFeatures_Square.png) |
| Example showing features touching Feature ID=0 (Black voxels) | Example showing features touching the outer surface of the bounding box |

## Parameters

None

## Required Geometry

Image

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| Cell Feature **Attribute Matrix** | Cell Feauture | N/A | The path to the cell feature **Attribute Matrix** |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | SurfaceFeatures | bool | (1) | Flag of 1 if **Feature** touches an outer surface or of 0 if it does not |

## Example Pipelines

+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.



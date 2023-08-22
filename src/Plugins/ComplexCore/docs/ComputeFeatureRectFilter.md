# Compute Feature Rect  #


## Group (Subgroup) ##

Reconstruction (Reconstruction)

## Description ##

This **Filter** computes the XYZ minimum and maximum coordinates for each **Feature** in a segmentation. This data can be important for finding the smallest encompassing volume. This values are given in **Pixel** coordinates.

|       | 0 | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|---|
| **0** | 0 | 0 | 1 | 0 | 0 |
| **1** | 0 | 0 | 1 | 1 | 0 |
| **2** | 0 | 1 | 1 | 1 | 1 |
| **3** | 0 | 0 | 1 | 1 | 0 |
| **4** | 0 | 0 | 0 | 0 | 0 |


If the example matrix above which represents a single feature where the feature ID = 1, the output of the filter would be:

    X Min = 1
    Y Min = 0
    Z Min = 0

    X Max = 4
    Y Max = 3
    Z Max = 0


## Parameters ##

N/A

## Required Geometry ##

N/A

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **FeatureIds** | FeatureIdsArrayName | int32_t | (1) | |
| **Feature Attribute Matrix** | N/A | Feature AttributeMatrix | N/A | The path to the cell feature **Attribute Matrix** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Feature Attribute Array** | FeatureRect | uint32 | (6) | Xmin, Ymin, Zmin, Xmax, Ymax, Zmax |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ComputeFeatureRectFilter
+ Displayed Name: Compute Feature Corners

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| feature_data_attribute_matrix_path | Feature Data Attribute Matrix | The path to the feature data attribute matrix associated with the input feature ids array | complex.DataGroupSelectionParameter |
| feature_ids_array_path | Feature Ids | Data Array that specifies to which Feature each Element belongs | complex.ArraySelectionParameter |
| feature_rect_array_path | Feature Rect | The feature rect calculated from the feature ids | complex.DataObjectNameParameter |


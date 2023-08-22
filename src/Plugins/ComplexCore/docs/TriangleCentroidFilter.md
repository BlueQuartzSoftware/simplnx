Generate Triangle Centroids 
============

## Group (Subgroup) ##

Surface Meshing (Misc)

## Description ##

This **Filter** computes the centroid of each **Triangle** in a **Triangle Geometry** by calculating the average position of all 3 **Vertices** that make up the **Triangle**.

## Parameters ##

None

## Required Geometry ##

Triangle

## Required Objects ##

None

## Created Objects ##
| Kind                     | Default Name  | Type   | Component Dimensions | Description                             |
|--------------------------|---------------|--------|----------------------|-----------------------------------------|
| **Face Attribute Array** | FaceCentroids | float32 | (1)                  | Specifies the centroid of each **Face** |


## Example Pipelines ##

+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: TriangleCentroidFilter
+ Displayed Name: Calculate Triangle Centroids

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| centroids_array_name | Created Face Centroids | The complete path to the array storing the calculated centroids | complex.DataObjectNameParameter |
| triangle_geometry_path | Triangle Geometry | The complete path to the Geometry for which to calculate the normals | complex.GeometrySelectionParameter |


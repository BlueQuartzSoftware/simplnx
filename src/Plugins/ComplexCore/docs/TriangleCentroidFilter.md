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

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: complex
+ Class Name: TriangleCentroidFilter
+ Displayed Name: Calculate Triangle Centroids

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| centroids_array_name | Created Face Centroids | The complete path to the array storing the calculated centroids | complex.DataObjectNameParameter |
| triangle_geometry_path | Triangle Geometry | The complete path to the Geometry for which to calculate the normals | complex.GeometrySelectionParameter |


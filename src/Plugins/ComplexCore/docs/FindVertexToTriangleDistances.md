Find Vertex to Triangle Distances 
=============

## Group (Subgroup) ##
Sampling (Spatial)

## Description ##
This **Filter** computes distances between points in a **Vertex Geoemtry** and triangles in a **Triangle Geoemtry**.  Specifically, for each point in the **Vertex Geometry**, the Euclidean distance to the closest triangle in the **Triangle Geoemtry** is stored.  This distance is *signed*: if the point lies on the side of the triangle to which the triangle normal points, then the distance is positive; otherwise, the distance is negative. Additionally, the ID the closest triangle is stored for each point.

## Parameters ##

None

## Required Geometry ##
Vertex and Triangle

## Required Objects ##
| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | None | N/A | N/A | **Data Container** holding the source **Vertex Geometry** |
| **Data Container** | None | N/A | N/A | **Data Container** holding the target **Triangle Geometry** |
| **Face Attribute Array** | None | float | (3)) | Normals for the target **Triangle Geometry** |

## Created Objects ##
| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Vertex Attribute Array** | Distances | float | (1) | Euclidean distance to the closest triangle for each point |
| **Vertex Attribute Array** | ClosestTriangleId | int32_t | (1) | Id of the closest triangle for each point |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
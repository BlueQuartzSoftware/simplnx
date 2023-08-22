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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: FindVertexToTriangleDistancesFilter
+ Displayed Name: Find Vertex to Triangle Distances

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| closest_triangle_id_array_path | Closest Triangle Ids Array | The array to store the ID of the closest triangle | complex.DataObjectNameParameter |
| distances_array_path | Distances Array | The array to store distance between vertex and triangle | complex.DataObjectNameParameter |
| triangle_data_container | Target Triangle Geometry | The triangle geometry to compare against | complex.GeometrySelectionParameter |
| triangle_normals_array_path | Triangle Normals | The triangle geometry's normals array | complex.ArraySelectionParameter |
| vertex_data_container | Source Vertex Geometry | The Vertex Geometry point cloud to map to triangles | complex.GeometrySelectionParameter |


Approximate Point Cloud Hull
=============

## Group (Subgroup) ##

Point Cloud (Geometry)

## Description ##

This **Filter** determines a set of points that approximates the surface (or *hull*) or a 3D point cloud represented by a **Vertex Geometry**.  The hull is approximate in that the surface points are not guaranteed to hve belonged to the original point cloud; instead, the determined set of points is meant to reprensent a sampling of where the 3D point cloud surface occurs. To following steps are used to approximate the the hull:

1. A structured rectilinear grid with user-defined resolution is overlaid on the point cloud.  
2. Each point is mapped to the voxel it occupies in the sampling grid.
3. For each voxel in the sampling grid:
    1. Each of its 26 neighbors is inspected to see if that neighbor contains any points.
    2. If the number of empty neighbors exceeds a user-defined threshold, the voxel is flagged as a "surface voxel".
4. For each voxel flagged as a "surface voxel", the coordinates of the points in that voxel are averaged to produce a new point that is inserted into the hull.

The above algorithm is significantly faster that other geoemtric approaches for determining a point cloud surface, but yields only an approximate solution.  Note that this approach is able of handling concavities in the point cloud, assuming the grid resolution is small enough to resolve any concavities.  In general, a grid resolution should be chosen small enough to resolve any surface features of interest.  The algorithm is also sensitive to the minimum number of empty neighbors parameter: consider modifying this parmater if the resulting hull is unsatisfactory.

Note that the resulting hull geometry does not inherit any **Attribute Arrays** from the original point cloud.

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Grid Resolution | float 3x | The resolution of the sampling grid |
| Minimum Number of Empty Neighbors | int | The minimum number of voxel neighbors that must contain 0 points for a voxel to be considered on the surface |

## Required Geometry ##

Vertex

## Required Objects ##
| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | None | N/A | N/A | **Data Container** holding the input **Vertex Geometry** |

## Created Objects ##
| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Data Container** | HullDataContainer | N/A | N/A | **Data Container** holding the approximated surface **Vertex Geometry** |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
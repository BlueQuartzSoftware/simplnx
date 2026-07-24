# Approximate Point Cloud Hull

## Group (Subgroup)

Point Cloud (Geometry)

## Description

This **Filter** determines a set of points that approximates the surface (or *hull*) of a 3D point cloud represented by a **Vertex Geometry**.  The hull is approximate in that the surface points are not guaranteed to have belonged to the original point cloud; instead, the determined set of points is meant to represent a sampling of where the 3D point cloud surface occurs.  The following steps are used to approximate the hull:

1. A structured rectilinear grid with user-defined resolution is overlaid on the point cloud.
2. Each point is mapped to the voxel it occupies in the sampling grid.
3. For each voxel in the sampling grid:
    1. Each of its 26 neighbors is inspected to see if that neighbor contains any points.
    2. If the number of empty neighbors exceeds a user-defined threshold, the voxel is flagged as a "surface voxel".
4. For each voxel flagged as a "surface voxel", the coordinates of the points in that voxel are averaged to produce a new point that is inserted into the hull.

The above algorithm is significantly faster than other geometric approaches for determining a point cloud surface, but yields only an approximate solution.  Note that this approach is capable of handling concavities in the point cloud, assuming the grid resolution is small enough to resolve any concavities.  In general, a grid resolution should be chosen small enough to resolve any surface features of interest.  The algorithm is also sensitive to the minimum number of empty neighbors parameter: consider modifying this parameter if the resulting hull is unsatisfactory.

### Parameter Guidance

- **Grid Resolution** -- the X, Y, and Z edge lengths of the sampling grid voxels, expressed in the same coordinate units as the input point cloud's vertex coordinates.
- **Minimum Number of Empty Neighbors** -- a dimensionless count in the range 0 to 26 (a voxel has 26 neighbors in a 3D grid). A voxel is flagged as a surface voxel when its number of empty neighbors exceeds this value. Larger values flag fewer voxels (only those more exposed on the surface).

Note that the resulting hull geometry does not inherit any **Attribute Arrays** from the original point cloud.

### Required Input Sources

- **Vertex Geometry** -- the input 3D point cloud, typically created by an import step or by a filter that produces a **Vertex Geometry**.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

# Approximate Point Cloud Hull

## Group (Subgroup)

Point Cloud (Geometry)

## Description

This **Filter** determines a set of points that approximates the surface (or *hull*) or a 3D point cloud represented by a **Vertex Geometry**.  The hull is approximate in that the surface points are not guaranteed to hve belonged to the original point cloud; instead, the determined set of points is meant to represent a sampling of where the 3D point cloud surface occurs. To following steps are used to approximate the hull:

1. A structured rectilinear grid with user-defined resolution is overlaid on the point cloud.  
2. Each point is mapped to the voxel it occupies in the sampling grid.
3. For each voxel in the sampling grid:
    1. Each of its 26 neighbors is inspected to see if that neighbor contains any points.
    2. If the number of empty neighbors exceeds a user-defined threshold, the voxel is flagged as a "surface voxel".
4. For each voxel flagged as a "surface voxel", the coordinates of the points in that voxel are averaged to produce a new point that is inserted into the hull.

The above algorithm is significantly faster that other geometric approaches for determining a point cloud surface, but yields only an approximate solution.  Note that this approach is able of handling concavities in the point cloud, assuming the grid resolution is small enough to resolve any concavities.  In general, a grid resolution should be chosen small enough to resolve any surface features of interest.  The algorithm is also sensitive to the minimum number of empty neighbors parameter: consider modifying this parameter if the resulting hull is unsatisfactory.

Note that the resulting hull geometry does not inherit any **Attribute Arrays** from the original point cloud.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.

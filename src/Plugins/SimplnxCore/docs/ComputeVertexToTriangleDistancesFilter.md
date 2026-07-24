# Compute Vertex to Triangle Distances

## Group (Subgroup)

Sampling (Spatial)

## Description

This **Filter** computes distances between points in a **Vertex Geometry** and triangles in a **Triangle Geometry**.  Specifically, for each point in the **Vertex Geometry**, the Euclidean distance to the closest triangle in the **Triangle Geometry** is stored.  This distance is *signed*: if the point lies on the side of the triangle to which the triangle normal points, then the distance is positive; otherwise, the distance is negative. Additionally, the ID of the closest triangle is stored for each point.

The computed distance is reported in the same length unit as the coordinates of the two input geometries (e.g., microns, millimeters); both geometries must share the same unit system for the result to be meaningful.

The sign of each distance depends on the **Triangle Normals**. For the sign to be correct, the normals must be present and consistently oriented across the mesh (all pointing to the same "outside"). Inconsistent or flipped normals will produce sign errors even when the distance magnitude is correct.

### Required Input Sources

- **Vertex Geometry** -- the point cloud whose distances are measured.
- **Triangle Geometry** -- the surface mesh measured against, typically produced by a surface-meshing filter such as [Quick Surface Mesh](QuickSurfaceMeshFilter.md) or read from a CAD mesh via [Read STL File](ReadStlFileFilter.md).
- **Triangle Normals** -- per-face normals on the **Triangle Geometry**, produced by [Compute Triangle Normals](TriangleNormalFilter.md); required for correct distance signs.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

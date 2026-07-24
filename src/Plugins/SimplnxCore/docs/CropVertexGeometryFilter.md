# Crop Geometry (Vertex)

## Group (Subgroup)

Geometry (Cropping/Cutting)

## Description

This **Filter** crops a **Vertex Geometry** (a point cloud) down to the points that lie inside a user-defined, axis-aligned bounding box. The box is specified by a *Min Pos* corner and a *Max Pos* corner. Both corners are given in **physical coordinate units** (the same length units as the vertex coordinates, e.g. microns), *not* in cell or index units.

A vertex is kept when its X, Y, and Z coordinates all fall within the box. The test is **inclusive** on every face of the box: a vertex is retained when its coordinate is greater than or equal to the corresponding *Min Pos* value and less than or equal to the corresponding *Max Pos* value. Vertices on the boundary planes are therefore kept, and vertices outside any face of the box are discarded.

Unlike cropping an **Image Geometry**, it is not known until run time how many vertices will survive the crop. Therefore this **Filter** creates a new **Vertex Geometry** to hold the cropped result rather than modifying the input in place. The user-selected vertex data arrays are copied into the new geometry with the tuples for any discarded vertices removed. The user must supply a name for the cropped geometry; all other copied objects keep the same names as in the original source.

*Note:* Because the number of surviving vertices cannot be known before run time, the new **Vertex Geometry** and all associated vertex data to be copied are initialized to size 0 and then grown to the final size during execution.

### Related Filters

- [Crop Geometry (Edge)](CropEdgeGeometryFilter.md) -- crops an **Edge Geometry** to a bounding box.
- [Crop Geometry (Image)](CropImageGeometryFilter.md) -- crops an **Image Geometry** by cell index ranges.

### Required Input Sources

- **Vertex Geometry to Crop** -- a **Vertex Geometry** (point cloud) to be cropped.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

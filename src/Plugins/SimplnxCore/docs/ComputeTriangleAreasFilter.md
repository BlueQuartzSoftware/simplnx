# Compute Triangle Areas

## Group (Subgroup)

Surface Meshing (Misc)

## Description

This filter computes the surface area of every **Triangle** in a **Triangle Geometry** (a surface mesh built from triangles) and stores the result as a per-triangle **Face Data** array.

### What This Measures and Why

Each triangle is defined by three corner **vertices** (nodes). The filter computes the area enclosed by those three vertices using

    Area = 1/2 * |AB| * |AC| * sin(O)

where *AB* and *AC* are two edge vectors of the triangle and *O* is the angle between them.

Per-triangle area is a basic mesh-quality and statistics input. It is used to weight other per-triangle quantities (for example, area-weighting triangle normals or curvature when computing feature-level averages) so that large triangles contribute proportionally more than small ones, and it can be summed to report the total surface area of a **Feature** or boundary.

### Units

The output area is reported in **squared geometry length units** (length^2). The value uses whatever length unit the **Triangle Geometry** vertices are stored in (for example, micrometers^2 if the mesh coordinates are in micrometers).

### Required Input Sources

- **Triangle Geometry** -- a surface mesh, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (07) Small IN100 Mesh Statistics
+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

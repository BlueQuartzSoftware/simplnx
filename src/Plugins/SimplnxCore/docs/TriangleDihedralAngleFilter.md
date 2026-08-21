# Calculate Triangle Minimum Dihedral Angle

## Group (Subgroup)

Surface Meshing (Misc)

## Description

This filter computes, for every **Triangle** in a **Triangle Geometry**, the smallest of its three interior corner angles, and stores that value as a per-triangle **Face Data** array.

### What This Measures and Why

Each triangle has three interior angles. This filter records the **minimum** of those three angles. (For a single flat triangle this minimum interior angle is what is referred to here as the "dihedral angle".) It is a standard **mesh-quality** metric:

- A well-shaped, near-equilateral triangle has a minimum angle close to 60°.
- A long, thin "sliver" triangle has a very small minimum angle (close to 0°). Slivers are numerically poorly conditioned and degrade downstream calculations (curvature, smoothing, finite-element meshing).

Coloring a surface mesh by this value is a quick way to locate sliver triangles that may need remeshing or smoothing.

### Units

The output angle is reported in **degrees** (0° to 60°; a value of 60° corresponds to a perfectly equilateral triangle).

### Required Input Sources

- **Triangle Geometry** -- a surface mesh, typically produced by a surface-meshing filter such as [Surface Nets](SurfaceNetsFilter.md) or [Quick Surface Mesh](QuickSurfaceMeshFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (07) Small IN100 Mesh Statistics
+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

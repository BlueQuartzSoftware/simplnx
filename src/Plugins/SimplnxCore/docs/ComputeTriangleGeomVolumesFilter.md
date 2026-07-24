# Compute Feature Volumes from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This filter computes the enclosed volume of each **Feature** in a **Triangle Geometry** (a surface mesh built from triangles). The result is the volume of each surface-meshed **Feature**, equivalently the volume of each unique closed polyhedron defined by the **Face Labels** array.

In a surface mesh, each triangle carries a **Face Labels** value: a pair of **Feature** Ids naming the two features on either side of that triangle. The set of triangles that reference a given **Feature** Id forms the closed shell of that feature. The **winding** of a triangle is the order in which its three corner vertices are listed; that order defines which way the triangle's surface normal points (inward vs. outward).

The volume of any generic closed polyhedron is computed as follows:

1. Triangulate each face of the polyhedron (here, each face is already a triangle in the **Triangle Geometry**).
2. Make the triangle normals consistent. This filter uses the convention where normals point inward. The actual winding stored in the **Triangle Geometry** is not modified.
3. For each triangular face, form a tetrahedron whose fourth vertex is the coordinate-system origin.
4. Compute the signed volume of each tetrahedron.
5. Sum the signed tetrahedra volumes to obtain the volume enclosed by the polyhedron.

### Watertight Requirement

This method only yields a correct value for a **watertight** (closed, gap-free) surface. The signed-tetrahedron sum relies on the feature's triangles forming a complete enclosing shell; if the surface has holes or is otherwise open, the computed volume is not meaningful. Surfaces produced by the standard surface-meshing filters are closed per feature and satisfy this requirement.

### Units

Volume is reported in **cubed geometry length units** (length^3), using whatever length unit the **Triangle Geometry** vertices are stored in (for example, micrometers^3 if the mesh coordinates are in micrometers).

### Required Input Sources

- **Triangle Geometry** -- a closed (watertight) surface mesh, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).
- **Face Labels** -- the per-triangle pair of **Feature** Ids, produced alongside the mesh by [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md) or [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md).

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

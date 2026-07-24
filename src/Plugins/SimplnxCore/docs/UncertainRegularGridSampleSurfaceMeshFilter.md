# Sample Triangle Geometry on Uncertain Regular Grid

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** "samples" a triangulated surface mesh onto a **rectilinear grid** (a regular grid of box-shaped **Cells**), but with "uncertainty" in the absolute position of each Cell. The uncertainty simulates the possible positioning error of a sampling probe. A **Cell** is a single volume element of the output grid, and a **Feature** is one labeled region of the surface mesh.

The user specifies the number of **Cells** along the X, Y, and Z directions, plus the resolution (spacing) and origin that define the grid. The grid resolution and origin are given in the same physical length units as the input surface mesh (typically microns). The three uncertainty values are also physical lengths in those same units. The sampling, with uncertainty, is performed by the following steps:

1. Determine the bounding box and **Triangle** list of each **Feature** by scanning all **Triangles** and noting the **Features** on either side of each **Triangle**.
2. For each **Cell** in the rectilinear grid, perturb its location by generating three random numbers in the range [-1, 1] and multiplying them by the three uncertainty values (one for each direction).
3. For each perturbed **Cell**, determine which bounding box(es) it falls in (*Note:* the bounding boxes of multiple **Features** can overlap).
4. For each bounding box a **Cell** falls in, test it against that **Feature's** **Triangle** list to determine whether the **Cell** lies within that n-sided polyhedron. (*Note:* if the surface mesh is **conformal** -- meaning adjacent features share exactly one common surface with no gaps or overlaps -- each **Cell** belongs to only one **Feature**; if not, the last **Feature** the **Cell** is found inside of *owns* the **Cell**.)
5. Assign the **Feature** number that the **Cell** falls within to the *Feature Ids* array in the new rectilinear grid geometry.

**Note that the unperturbed grid is where the *Feature Ids* actually live, but the perturbed locations are where the Cells are sampled from. Essentially, the *Feature Ids* are stored where the user *thinks* the sampling took place, not where it actually took place.**

For the variant that samples onto a regular grid without positional uncertainty, see the sibling filter [Sample Triangle Geometry on Regular Grid](RegularGridSampleSurfaceMeshFilter.md), which shares the same grid (dimensions, origin, spacing) parameters.

### Required Input Sources

- **Triangle Geometry** -- the surface mesh to sample, typically produced by a surface-meshing filter such as [Create Surface Mesh (Surface Nets)](SurfaceNetsFilter.md) or [Create Surface Mesh (QuickMesh)](QuickSurfaceMeshFilter.md).
- **Face Labels** -- the per-face 2-component array identifying the **Features** on either side of each triangle, produced by the same surface-meshing filter.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

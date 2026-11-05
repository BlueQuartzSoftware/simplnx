# Compute Feature Shapes (Triangle Geometry)

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** measures the shape of each **Feature**, a mesh region identified by its face labels, in a **Triangle Geometry**. It integrates tetrahedral moments to determine principal axes and Omega3. It measures an axis length as a half-span between mesh intersections along each principal axis.

### Axis Lengths and Aspect Ratios

For each unit principal axis, the filter traces two rays from the feature centroid in opposite directions. Let `d+` and `d-` be the distances to the farthest accepted hit in each direction. A direction with no hit contributes zero. The reported length is `(d+ + d-)/2`, in the geometry's length units.

This measure includes gaps between hits in a nonconvex feature. If the centroid lies outside the mesh, the same formula applies; the result is not the occupied length. For example, if one direction has no hit and the other has a farthest hit at distance 4, the length is 2. Reversing an axis direction gives the same length.

The three axes are ordered by increasing inertia eigenvalue. The first axis has the smallest inertia eigenvalue. Each length stays paired with its axis direction. The measured lengths are not sorted, so `a >= b >= c` is not guaranteed. The dimensionless aspect ratios are `b/a` and `c/a`. If any length is zero, all three lengths and both ratios are set to `-1`.

**Axis Euler Angles** are in radians. They describe a proper rotation whose rows follow the ordered principal-axis lines. The third axis direction is reversed when necessary to make the frame right-handed. The direction sign does not change a half-span. Symmetric shapes can have equivalent principal-axis orientations with different Euler angles.

### Differences from the Image Geometry Version

The [Compute Feature Shapes (Image Geometry)](ComputeShapesFilter.md) filter derives ellipsoid lengths from voxel moments. This filter measures half-spans against the mesh. The two filters can therefore report different lengths and aspect ratios. Voxel results also depend on the sampling resolution.

### Mesh Interpretation

The moment integration treats each feature surface as an enclosed region. Holes, gaps, or inconsistent face orientation can make the shape measurements inaccurate. The Euler characteristic output provides a topology indicator for each mesh region.

### Required Input Sources

- **Triangle Geometry** with **Face Labels** -- produced by a surface meshing filter such as [Quick Surface Mesh](../SimplnxCore/QuickSurfaceMeshFilter.md).
- **Feature Centroids** -- produced by [Compute Feature Centroids](../SimplnxCore/ComputeFeatureCentroidsFilter.md), typically on the voxelized feature data before meshing.

## Algorithm

1. Construct a signed tetrahedron from each feature boundary triangle and the supplied feature centroid. Use the face label side to select the triangle orientation without changing the mesh winding.
2. Integrate the tetrahedron's volume and second moments analytically. Sum these contributions over the feature surface and normalize the moments by the signed volume.
3. Form the inertia tensor from the second moments. Compute its eigenvalues and eigenvectors, then order the principal axes by increasing inertia eigenvalue.
4. Convert the principal frame to Euler angles after correcting its handedness.
5. Trace rays in both directions along each principal axis against the feature's triangles. Retain the farthest hit in each direction and calculate the half-span. A hit is accepted when its ray parameter exceeds the existing numerical epsilon.
6. Calculate the two aspect ratios from the ordered lengths. Keep distances, half-spans, and ratio calculations in double precision, then write the outputs in single precision.
7. Calculate the normalized Omega3 descriptor from the integrated volume and the diagonal inertia terms. Omega3 is dimensionless. This calculation does not fit an ellipsoid or subdivide the tetrahedra.

% Auto generated parameter table will be inserted here

**Face Feature Centroids** must have 3 components.

## References

[1] Representation and Reconstruction of Three-dimensional Microstructures in Ni-based Superalloys, AFOSR
FA9550-07-1-0179 Final Report, 20 Dec 2010.

[2] On the use of moment invariants for the automated classification of 3-D particle shapes, J. MacSleyne, J.P. Simmons
and M. De Graef, Modeling and Simulations in Materials Science and Engineering, 16, 045008 (2008).

[3] n-Dimensional Moment Invariants and Conceptual Mathematical Theory of Recognition n-Dimensional Solids, Alexander G.
Mamistvalov, IEEE TRANSACTIONS ON PATTERN ANALYSIS AND MACHINE INTELLIGENCE, VOL. 20, NO. 8, AUGUST 1998, p. 819-831.

[4] M. Groeber, M. Uchic, D. Dimiduk, and S. Ghosh. A Framework for Automated Analysis and Simulation of 3D
Polycrystalline Microstructures, Part 1: Statistical Characterization Acta Materialia, 56 (2008), 1257-1273.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

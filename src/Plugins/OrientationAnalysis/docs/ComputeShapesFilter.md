# Compute Feature Shapes (Image Geometry)

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** characterizes the 3D shape of each **Feature** (grain or particle) by fitting a best-fit ellipsoid to its voxels. The result is a set of shape descriptors including axis lengths, aspect ratios, axis orientations, and a shape invariant (Omega3).

### What This Filter Produces

- **Semi-Axis Lengths** -- The half-lengths of the three principal axes of the best-fit ellipsoid (a &ge; b &ge; c). These describe the size and elongation of the grain.
- **Aspect Ratios** -- The ratios b/a and c/a, which describe the grain's shape independent of its size. An equiaxed (roughly spherical) grain has aspect ratios near 1.0; an elongated grain has lower values.
- **Axis Euler Angles** -- The orientation of the ellipsoid's principal axes in the sample reference frame, stored as Euler angles. These describe which direction the grain is elongated.
- **Omega3** -- A dimensionless shape invariant derived from the second-order moments [2]. Omega3 is 1.0 for a perfect sphere and decreases for shapes that deviate from spherical. It provides a single-number summary of how "round" a grain is, independent of its size or orientation.

### How This Filter Works

1. For each **Cell**, compute the x, y, and z distances from the cell center to the centroid of its parent **Feature**
2. Accumulate the second-order moment terms (Ixx, Iyy, Izz, Ixy, Ixz, Iyz) for all **Cells** in each **Feature**
3. Solve for the eigenvalues and eigenvectors of the resulting 3x3 moment tensor for each **Feature**
4. Convert eigenvalues to ellipsoid semi-axis lengths using the relationship between principal moments and axis lengths [4]
5. Compute aspect ratios, axis orientation angles, and the Omega3 shape invariant

### Note

For shape analysis on triangle geometry meshes rather than voxelized image geometry, see the [Compute Feature Shapes (Triangle Geometry)](ComputeShapesTriangleGeomFilter.md) filter.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](../SimplnxCore/ScalarSegmentFeaturesFilter.md).
- **Feature Centroids** -- produced by [Compute Feature Centroids](../SimplnxCore/ComputeFeatureCentroidsFilter.md).

% Auto generated parameter table will be inserted here

## References

[1] Representation and Reconstruction of Three-dimensional Microstructures in Ni-based Superalloys, AFOSR FA9550-07-1-0179 Final Report, 20 Dec 2010.

[2] J. MacSleyne, J.P. Simmons, and M. De Graef. On the use of moment invariants for the automated classification of 3-D particle shapes. *Modeling and Simulations in Materials Science and Engineering*, 16, 045008 (2008).

[3] A.G. Mamistvalov. n-Dimensional Moment Invariants and Conceptual Mathematical Theory of Recognition n-Dimensional Solids. *IEEE Transactions on Pattern Analysis and Machine Intelligence*, 20(8), 819-831 (1998).

[4] M. Groeber, M. Uchic, D. Dimiduk, and S. Ghosh. A Framework for Automated Analysis and Simulation of 3D Polycrystalline Microstructures, Part 1: Statistical Characterization. *Acta Materialia*, 56, 1257-1273 (2008).

## Example Pipelines

+ `(03) Small IN100 Morphological Statistics`
+ `(06) SmallIN100 Synthetic`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

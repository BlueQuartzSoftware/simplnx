# Compute Feature Shapes from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Warning

This filter has two caveats.

Firstly, the axial lengths of this filter will be different than those produced by the voxelized version of this filter. This is for two reasons:

- The sampling rate and density for the grid that was used to voxelize the mesh. See *Sample Triangle Geometry on Regular Grid* (RegularGridSampleSurfaceMesh).
- This filter determines axial lengths via distance from feature centroid to mesh intersection points along each of the principle axes. This means they are relative to the mesh itself rather than the grid it exists in.

Secondly, shapes that exhibit rotational symmetry (e.g. cube, sphere, regular octahedron, etc.) may have different Euler Angles than those of the voxelized implementation, but they are functionally identical. This is more prevalent in meshes with less traingles, but this is seemly due to the fact the tested shapes are more uniform in low-poly. It is presumed that fiducial markers will stabilize ouputs for these specific shapes.

## Description

This **Filter** calculates the second-order moments of each enclosed **Feature** in a **Triangle Geometry**. The
second-order moments allow for the determination of the *principal axis lengths, principal axis directions, aspect
ratios and moment invariant Omega3s*. The *principal axis lengths* are those of a "best-fit" ellipsoid. The algorithm
for determining the moments and these values is as follows:

1. For each **Triangle** on the bounding surface of a **Feature**, construct a tetrahedron whose fourth vertex is the
   centroid of the **Feature**, ensuring normals are consistent (this **Filter** uses the convention where normals point
   inwards; note that the actual winding of the **Triangle Geometry** is not modified)
2. Subdivide each constructed tetrahedron into 8 smaller tetrahedra
3. For each subdivided tetrahedron, compute the distance from that tetrahedron's centroid to the centroid of the
   parent **Feature**
4. For each subdivided tetrahedron, calculate Ixx, Iyy, Izz, Ixy, Ixz and Iyz using the x, y and z distances determined
   in step 1
5. Use the relationship of *principal moments* to the *principal axis lengths* for an ellipsoid, which can be found
   in [4], to determine the *Axis Lengths*
6. Calculate the *Aspect Ratios* from the *Axis Lengths* found in step 5.
7. Determine the Euler angles required to represent the *principal axis directions* in the *sample reference frame* and
   store them as the **Feature**'s *Axis Euler Angles*.
8. Calculate the moment invariant Omega3 as defined in [2] and is discussed further in [1] and [3]

*Note:* Due to the method used to subdivide the tetrahedra, some sharp corners of shapes may not be properly
represented, resulting in inaccurate Omega3 values. This problem is especially apparent for perfect rectangular prisms,
but any shape with clear sharp corners may be affected.

% Auto generated parameter table will be inserted here

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

## Exemplars For Test/Validation

Note that the `AxisEulerAngles` are in radians. The `_NUMBER` is the component index. The names correspond to files in the test data. The test data also contains a `.dream3d` file (and corresponding `.xdmf` file) that has the preprocessed input data as well as two vertex geometries that correspond to the axial intersection points for both this filter (named `stl`) and the voxelized implementation (named `voxelized`) to demonstrate the difference between the two results. See `validation` folder in the associated test file. The code to generate this was not production level so it was not included in the release of this filter.

| Index | Exemplar Omega3s | Exemplar AxisEulerAngles_0 | Exemplar AxisEulerAngles_1 | Exemplar AxisEulerAngles_2 | Exemplar AxisLengths_0 | Exemplar AxisLengths_1 | Exemplar AxisLengths_2 | Centroids_0 | Centroids_1 | Centroids_2 | STL File List |
|---|---|---|---|---|---|---|---|---|---|---|--------|
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 'JUNK' |
| 1 | 0.78787351 | 6.2519455 | 0.46384287 | 1.5707964 | 0.55903065 | 0.5 | 0.5590716 | 11 | 1.5 | 0.5 | '101_cube.stl' |
| 2 | 0.90795749 | 1.5712752 | 1.5711994 | 4.711997 | 0.50000107 | 0.50000107 | 0.50000536 | 7.9999948 | 1.500001 | 0.50000101 | '102_rounded_cube.stl' |
| 3 | 0.99998772 | 3.1415451 | 1.5707964 | 1.5707963 | 0.4999963 | 0.50000113 | 0.49999821 | 5.0000029 | 1.4999999 | 0.50000352 | '103_sphere.stl' |
| 4 | 0.81056947 | 0 | 0 | 0 | 0.5 | 0.5 | 0.5 | 2 | 1.5 | 0.5 | '104_octahedron.stl' |
| 5 | 0.78787351 | 0 | 1.5707964 | 4.712389 | 1.5 | 1 | 0.5 | 11 | 3.5 | 1.5 | '105_rectangular_prism.stl' |
| 6 | 0.90794492 | 2.3219979e-10 | 1.5707964 | 1.5707964 | 1.5000002 | 1.0000105 | 0.50000501 | 8.0000105 | 3.500005 | 1.4999998 | '106_rounded_cube_elongated.stl' |
| 7 | 0.99998951 | 3.1415927 | 1.5707964 | 1.5707964 | 1.499998 | 0.99999982 | 0.50000066 | 5 | 3.4999993 | 1.5000019 | '107_ellipsoid.stl' |
| 8 | 0.4052847 | 2.9784336 | 1.2776501 | 2.7590237 | 0.72980016 | 0.63307023 | 0.90308428 | 11.949251 | 5.9832501 | 0.56225002 | '108_tetrahedron.stl' |
| 9 | 0.86399871 | 4.7129421 | 1.5707964 | 1.5707964 | 1.25 | 1.0001171 | 1.0003718 | 2.0074062 | 6.1888757 | 1.25 | '109_cylinder.stl' |
| 10 | 0.96753073 | 4.712389 | 1.7409153 | 4.712389 | 1.0468618 | 1.1038886 | 1.2814574 | 8.2501669 | 6.1889997 | 1.03175 | '110_icosahedron.stl' |
| 11 | 0.95603704 | 1.5717114 | 2.5809844 | 4.7131243 | 0.95104223 | 0.95071417 | 0.94695413 | 4.8930006 | 6.1890001 | 0.80900002 | '111_dodecahedron.stl' |
| 12 | 0.81056947 | 3.1415927 | 1.5707964 | 1.5707964 | 1.5 | 1 | 0.5 | 2 | 3.5 | 1.5 | '112_octahedron.stl' |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

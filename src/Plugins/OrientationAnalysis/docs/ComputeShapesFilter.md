# Compute Feature Shapes (Image Geometry)

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the second-order moments of each **Feature** in order to determine the *principal axis lengths, principal axis directions, aspect ratios and moment invariant Omega3s*.  The *principal axis lengths* are those of a "best-fit" ellipsoid.  The algorithm for determining the moments and these values is as follows:

1. For each **Cell**, determine the x, y and z distance to the centroid of the **Feature** that owns the **Cell**
2. For each **Cell**, calculate Ixx, Iyy, Izz, Ixy, Ixz and Iyz using the x, y and z distances determined in step 1.
3. Sum the individual Ixx, Iyy, Izz, Ixy, Ixz and Iyz values for all **Cells** belonging to the same **Feature**
4. Find the *eigenvalues* and *eigenvectors* of the *3x3* symmetric matrix defined by the *6* values calculated in step 3 for each **Feature**
5. Use the relationship of *principal moments* to the *principal axis lengths* for an ellipsoid, which can be found in [4], to determine the *Semi-Axis Lengths*
6. Calculate the *Aspect Ratios* from the *Semi-Axis Lengths* found in step 5.
7. Determine the Euler angles required to represent the *principal axis directions* in the *sample reference frame* and store them as the **Feature**'s *Axis Euler Angles*.
8. Calculate the moment variant Omega3 as definied in [2] and is discussed further in [1] and [3]

### Numerical Details

Step 1 does not use a single sample point per **Cell**. Each **Cell** is split into octants and
sampled at the eight points offset by +/- *spacing/4* from the **Cell** center (four points offset by
+/- *spacing/4* in 2D). This eight-point quadrature is what keeps a single-**Cell** **Feature** from
having zero moments, but it is not the exact integral over the **Cell** volume: it contributes a
self-moment of *d^2/16* per axis where the exact integral would contribute *d^2/12*. For an
axis-aligned box of *N* **Cells** along an axis the resulting per-**Cell** second moment factor is
therefore *(N^2 - 1)/12 + 1/16* rather than *N^2/12*. The difference is negligible for large
**Features** and dominant for **Features** only one or two **Cells** across.

Moments are accumulated in double precision, but the per-**Cell** contributions and all of the
output arrays are single precision, and the eigen decomposition is performed on a single-precision
3x3 matrix. Expect roughly 1e-6 relative agreement between two implementations of this algorithm,
not bit equality.

*Omega3s* is clamped to 1.0. A compact **Feature** only a few **Cells** across can produce a raw
value above 1 purely from the quadrature above, and such **Features** will all report exactly 1.0.

### Semi-Axis Ordering and Axis Euler Angles

*Axis Lengths* are always ordered *a >= b >= c*, so component 0 is the longest semi-axis. The two
*Aspect Ratios* are *b/a* and *c/a*, and for a three-dimensional **Feature** with **Cells** both lie
in *(0, 1]*. Two cases write 0 instead: in the two-dimensional formulation *c* is always 0, so
*Aspect Ratios* component 1 (*c/a*) is always 0 (see Two-Dimensional Geometries); and a **Feature**
with no **Cells** has both *Aspect Ratios* written as 0 (see Features With No Cells).

The *Axis Euler Angles* are the Bunge (ZXZ) angles of a matrix whose rows are the principal axis
directions: row 0 is the direction of the longest semi-axis *a* and row 2 is the direction of the
shortest semi-axis *c*. The sign of each eigenvector is chosen arbitrarily by the eigen solver, so
these angles are only defined up to the sign of each axis; compare axis *directions* (for example by
the absolute value of a dot product), never the Euler angles component by component. A right-handed
frame is not guaranteed: the handedness correction in the implementation tests
`OrientationMatrix::isValid()` for a result of 0, which that routine never returns, so the
correction never fires. Only the third row of the matrix, the shortest-axis direction, survives the
round trip through the Euler angles unchanged when the frame happens to be left-handed. This is a
known latent issue carried over from DREAM3D 6.5; it is documented rather than changed because
fixing it would alter reported values. (Evidence: source-derived from
`EbsdLib/Orientation/OrientationMatrix.hpp`, which returns only 1, -1, -2 or -3.)

### Two-Dimensional Geometries

When any dimension of the selected **Image Geometry** is 1 **Cell**, a two-dimensional formulation
runs instead. In that case:

- Only two semi-axes exist. *Axis Lengths* component 2 and *Aspect Ratios* component 1 are 0.
- *Omega3s* is a three-dimensional invariant and is **not** computed; every value is 0.
- *Axis Euler Angles* reduce to a single in-plane angle in component 0, measured from the first
  in-plane axis, with components 1 and 2 set to 0. Unlike the 3D case this angle comes straight
  from the moment matrix, so it is well defined and directly comparable.
- The two in-plane axes are whichever two axes are not flat, so an X-normal or Y-normal slab is
  handled with its own spacings, origin components and **Centroids** components.

### Features With No Cells

If the **Cell Feature Attribute Matrix** has more tuples than there are **Features** actually
present in the **Feature Ids** array, the unused ids have no moments at all. All of their axis
outputs -- *Axis Lengths*, *Aspect Ratios*, *Axis Euler Angles* and *Omega3s* -- are written as 0.

### Differences From DREAM3D 6.5.171

The 6.5.171 `FindShapes` filter sampled the voxel corner, `origin + k * spacing`, rather than the
voxel center, while the **Centroids** it was handed came from `FindFeatureCentroids`, which averages
voxel centers. Every offset therefore carried a half-**Cell** bias, which both inflated the diagonal
moments and made the off-diagonal moments non-zero for axis-aligned **Features**, tilting the
reported principal axes. The 6.5.171 two-dimensional path additionally assumed the in-plane axes
were always X and Y. This **Filter** samples **Cell** centers and maps the in-plane axes correctly,
so its *Omega3s*, *Axis Lengths*, *Aspect Ratios* and *Axis Euler Angles* differ from 6.5.171 for
every **Feature**, and its *Volumes* differ for X-normal and Y-normal slabs. *Volumes* agree
elsewhere. A surgical patch to a local build of the legacy `FindShapes` source making the same two corrections
reproduces this **Filter**'s output on the V&V fixture set, confirming that the difference is
entirely accounted for by these two defects. The two **Features** with no **Cells** in that fixture
set are excluded from that match and are intentionally divergent: legacy leaves NaN in *Axis Lengths*
and an arbitrary orientation in *Axis Euler Angles* for them, where this **Filter** now writes 0 (see
Features With No Cells).

% Auto generated parameter table will be inserted here

## References ##

[1] Representation and Reconstruction of Three-dimensional Microstructures in Ni-based Superalloys, AFOSR FA9550-07-1-0179 Final Report, 20 Dec 2010.

[2] On the use of moment invariants for the automated classifcation of 3-D particle shapes, J. MacSleyne, J.P. Simmons and M. De Graef, Modeling and Simulations in Materials Science and Engineering, 16, 045008 (2008).

[3] n-Dimensional Moment Invariants and Conceptual Mathematical Theory of Recognition n-Dimensional Solids, Alexander G. Mamistvalov, IEEE TRANSACTIONS ON PATTERN ANALYSIS AND MACHINE INTELLIGENCE, VOL. 20, NO. 8, AUGUST 1998, p. 819-831.

[4] M. Groeber, M. Uchic, D. Dimiduk, and S. Ghosh.    A Framework for Automated Analysis and Simulation of 3D Polycrystalline Microstructures, Part 1: Statistical Characterization    Acta Materialia, 56 (2008), 1257-1273.

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

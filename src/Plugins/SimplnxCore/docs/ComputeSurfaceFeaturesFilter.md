# Compute Surface Features

## Group (Subgroup)

Generic (Spatial)

## Description

This **Filter** determines whether a **Feature** touches an outer surface of the sample. This is accomplished by simply querying the **Feature** owners of the **Cells** that sit at either . Any **Feature** that owns one of those **Cells** is said to touch an outer surface and all other **Features** are said to not touch an outer surface of the sample.

This **Filter** determines whether a **Feature** touches an outer *Surface* of the sample volume. A **Feature** is considered touching the *Surface* of the sample if either of the following conditions are met:

+ Any cell location is x<sub>min</sub>, x<sub>max</sub>, y<sub>min</sub>, y<sub>max</sub>, z<sub>min</sub> or z<sub>max</sub>
+ Any cell has **Feature ID = 0** as a neighbor.

The output of this filter is a **Feature** level array of booleans where 0=Interior/Not touching and 1=Surface/Touching.

### WARNING - Feature ID=0 Voxels

If there are voxels within the volume that have **Feature ID=0** then any feature touching those voxels will be considered a *Surface* feature.

### WARNING - Fixed bugs

The version of this filter in legacy DREAM3D-NX (version 6.x) had two bugs: one that indexed into neighboring features incorrectly [DREAM3D-NX repo issue #988](https://github.com/BlueQuartzSoftware/DREAM3D/issues/988), and another that incorrectly labeled feature 0 as a surface feature when feature 0 exists in the feature ids array [DREAM3D-NX repo issue #989](https://github.com/BlueQuartzSoftware/DREAM3D/issues/989). Both of these bugs have been fixed in this new version.

### 2D Image Geometry

If the structure/data is actually 2D, then the dimension that is planar is not considered and only the **Features** touching the edges are considered surface **Features**.

### Example Output

|       |        |
|-------|--------|
| ![ComputeSurfaceFeatures_Cylinder](Images/ComputeSurfaceFeatures_Cylinder.png) |  ![ComputeSurfaceFeatures_Square](Images/ComputeSurfaceFeatures_Square.png) |
| Example showing features touching Feature ID=0 (Black voxels) "Mark Feature 0 Neighbors" is **ON** | Example showing features touching the outer surface of the bounding box |

## Algorithm

The filter examines every voxel in the image geometry and determines whether the feature owning that voxel qualifies as a "surface feature." A feature is marked as a surface feature the first time any of its voxels meets the surface criteria; once marked, subsequent voxels of that feature are skipped (short-circuit optimization).

A voxel qualifies its feature as a surface feature if:

1. The voxel is located on the outer boundary of the image geometry (x, y, or z is at its minimum or maximum value), **OR**
2. Any of the voxel's face neighbors has Feature ID = 0 (when **Mark Feature 0 Neighbors** is enabled).

For 2D geometries (where one dimension has size 1), the degenerate dimension is collapsed and only the 4 in-plane neighbors are checked.

### In-Core Algorithm (Direct)

The in-core variant uses separate code paths for 3D and 2D geometries. For 3D, it iterates all voxels in Z-Y-X order and checks 6 face neighbors via operator[] on the FeatureIds DataStore. For 2D, it determines which dimension is degenerate and iterates the non-degenerate plane, checking 4 neighbors. This approach works well when all data is resident in memory.

### Out-of-Core Algorithm (Scanline)

When the FeatureIds array is stored out-of-core in chunked format, the in-core algorithm's random neighbor lookups would trigger chunk load/evict cycles. The Scanline variant reads one complete native Z-slice at a time using sequential bulk I/O, maintaining a 3-slice rolling window (prevSlice, curSlice, nextSlice).

For 3D geometries, the neighbor lookups map directly to the rolling window buffers. For 2D geometries, the algorithm still iterates the native Z-Y-X grid but remaps coordinates to the logical 2D plane:

+ **Degenerate Z** (most common): All data fits in a single Z-slice; all 4 neighbors come from curSlice.
+ **Degenerate X or Y**: The remapped-Y direction maps to the native Z axis, so +/-Y neighbors come from the adjacent Z-slice buffers.

The feature-level SurfaceFeatures output is cached in a local vector during processing and bulk-written once at the end, avoiding per-voxel random writes to the OOC store.

### Performance

The in-core and out-of-core variants produce identical results. The algorithm dispatch is automatic based on the storage type of the FeatureIds array. Memory overhead for the Scanline variant is 3 Z-slices of int32 (for the rolling window) plus a small feature-level vector.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

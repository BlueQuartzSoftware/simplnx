# Erode/Dilate Coordination Number

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** will smooth the interface between *good* and *bad* data. The user can specify a *coordination number*,
which is the number of neighboring **Cells** of opposite type (i.e., *good* or *bad*) compared to a given **Cell** that
is acceptable. For example, a single *bad* **Cell** surrounded by *good* **Cells** would have a *coordination number* of
*6*. The number entered by the user is actually the maximum tolerated *coordination number*. If the user entered a value
of *4*, then all *good* **Cells** with 5 or more *bad* neighbors and *bad* **Cells** with 5 or more *good* neighbors
would be removed. After **Cells** with unacceptable *coordination number* are removed, then the neighboring **Cells**
are *coarsened* to fill the removed **Cells**.

By default, the **Filter** will only perform a single iteration and will not concern itself with the possibility that
after one iteration, **Cells** that were acceptable may become unacceptable by the original *coordination number*
criteria due to the small changes to the structure during the *coarsening*. The user can opt to enable the *Loop Until
Gone* parameter, which will continue to run until no **Cells** fail the original criteria.

| Before Filter                      | After Filter                       |
|--------------------------------------|--------------------------------------|
| ![](Images/ErodeDilateCoordinationNumber_Before.png) | ![](Images/ErodeDilateCoordinationNumber_After.png) |

## Algorithm

For each voxel on a good/bad boundary (where "good" means FeatureId > 0 and "bad" means FeatureId == 0), the algorithm counts how many of its 6 face-connected neighbors belong to the opposite class. This count is the voxel's **coordination number**.

A high coordination number means a voxel is mostly surrounded by the opposite type and is likely a boundary artifact or noise. For example, a single bad voxel completely surrounded by good voxels has a coordination number of 6.

### Processing Steps

1. For each boundary voxel, compute the coordination number by counting opposite-type face neighbors.
2. Among those opposite-type neighbors, identify the most common FeatureId.
3. If the coordination number meets or exceeds the user's threshold, mark the voxel to be replaced by the most common neighbor's data.
4. After scanning the entire volume, apply all marked replacements.

If **Loop Until Gone** is enabled, the algorithm repeats this process until no voxels exceed the coordination number threshold. Each pass may create new boundary conditions that expose previously acceptable voxels, so multiple passes can be necessary to fully smooth the interface.

All sibling data arrays in the same Attribute Matrix (except those in the user's ignored list) are updated along with the FeatureIds to maintain data consistency.

### Performance

This algorithm is optimized for both in-memory and out-of-core (OOC) data stores. When data resides on disk in chunked format, random voxel access can cause expensive chunk load/evict cycles. The implementation avoids this by:

- **Sequential Z-slice processing**: The volume is scanned one Z-slice at a time, aligning with typical chunk boundaries.
- **3-slice rolling window**: Three adjacent Z-slices of FeatureIds are held in memory for face-neighbor lookups without per-voxel store access.
- **Conditional deferred writes**: Only voxels whose coordination number meets the threshold are transferred, and writes are batched per Z-slice.
- **O(sliceSize) memory**: Per-slice mark and coordination arrays replace full-volume arrays, keeping peak memory proportional to a single Z-slice.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.

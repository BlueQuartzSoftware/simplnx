# Compute Feature Sizes

## Group (Subgroup)

Statistics (Morphological)

## Description

**Note:** Regarding using an *Image Geometry* as input, if you have a `1` in any of the dimensions it will be turned into a 2D calculation instead. See *Image Geometry Additional Considerations* section for more details.

This **Filter** calculates the sizes, volumes/areas, and equivalent diameters of all **Features** in an **Image Geometry** or **Rectilinear Grid Geometry**.

To do so, the **Filter** simply iterates through all **Elements** querying for the **Feature** that owns them and keeping a tally for each **Feature**. The tally is then stored as *NumElements* and the *Volume* and *EquivalentDiameter* are also calculated (and stored) by knowing the volume of each **Element**.

![Fig. 1: The Equivalent (Spherical) Diameter of a feature is the diameter of the sphere whose volume equals the feature's volume (number of cells × voxel volume).](Images/ComputeFeatureSizes_EquivalentDiameter.png)

Note here that **Image Geometry** will always be faster than its equivalent **Rectilinear Grid Geometry** because we leverage the uniformity of voxel sizes to perform volume/area calculations at the feature level rather than the cell level.

During the computation of the **Feature** sizes, the size of each individual **Element** is computed and stored in the corresponding **Geometry**. By default, these sizes are deleted after executing the **Filter** to save memory. If you wish to store the **Element** sizes, select the *Generate Missing Element Sizes* option. The sizes will be stored within the **Geometry** definition itself, not as a separate **Attribute Array**.

## Algorithm

### What the filter computes

Each **Feature** occupies some number of **Cells** (voxels) in the input grid. The filter produces three arrays indexed by Feature ID:

- **NumElements** — the number of voxels belonging to each feature (int32).
- **Volume** (or **Area** in 2D) — the physical volume (area) of each feature (float32).
- **EquivalentDiameter** — the diameter of the sphere (or circle in 2D) that would have the same volume (area) as the feature (float32).

Equivalent spherical diameter `d` is computed from volume `V` using `V = (4π/3)·r³`, giving `d = 2·(V / (4π/3))^(1/3)`. The 2D equivalent circular diameter uses `A = π·r²`, giving `d = 2·(A/π)^(1/2)`.

### Image Geometry path

An **Image Geometry** has uniform voxel spacing, so the volume of every voxel is the same: `V_voxel = dx · dy · dz`. The filter exploits this to skip per-voxel volume computations:

1. **Count voxels per feature.** Stream the per-cell Feature IDs array in 256K-tuple chunks. For each chunk:
   - Bulk-read the chunk via `copyIntoBuffer()` into a 1 MB RAM buffer.
   - Loop over the buffer, incrementing `featureVoxelCounts[id]` for each voxel's Feature ID.
   - The counter array is indexed by feature (thousands of entries, ~8 bytes each) and easily fits in L2 cache, so increments cost a few cycles.
2. **Compute per-feature outputs** in a single pass over the feature-level arrays:
   - `NumElements[f] = featureVoxelCounts[f]`
   - `Volume[f] = featureVoxelCounts[f] * V_voxel`
   - `EquivalentDiameter[f] = 2·cbrt(Volume[f] / (4π/3))`

A 2D fallback (when any one of the three dimensions equals 1) computes area and **Equivalent Circular Diameter** using `2·sqrt(Area/π)` instead. The filter errors out in preflight if two or more dimensions equal 1, because the intended orientation for the 1D/2D scaling is ambiguous in that case.

### Rectilinear Grid Geometry path

A **Rectilinear Grid** has per-voxel spacing, so cell volumes vary. The filter can't use the uniform-spacing shortcut and must sum each feature's member voxel volumes explicitly:

1. **Cache element sizes** via the geometry's `findElementSizes()` helper (stored as a cell-level array).
2. **Count voxels and sum volumes in lockstep.** Stream both the Feature IDs array and the element sizes array in 256K-tuple chunks (~2 MB working set total):
   - Bulk-read matching chunks of Feature IDs and element sizes.
   - For each voxel in the chunks:
     - Increment `featureVoxelCounts[id]`.
     - Accumulate `featureVolumes[id] += elementSize[i]` using **Kahan summation** (tracks per-feature compensator terms to correct floating-point rounding error). Kahan is necessary because summing billions of float32 volumes in native precision would lose low-order bits, especially for large features.
3. **Compute per-feature outputs** using `featureVolumes[f]` directly (no post-multiplication).

If `Save Element Sizes` is off, the element sizes array created in step 1 is deleted at the end to save memory.

### Why 256K chunks

The voxel-counting pass is I/O-bound on OOC-backed Feature IDs. Each `copyIntoBuffer()` call carries fixed HDF5 chunk-lookup overhead; the compute (a single indexed increment) is memory-bandwidth-bound on a counter that easily fits in L2. On a 2 B-voxel volume, 256 K-tuple chunks reduce the call count from ~30 K (at the old 64 K chunk) to ~7.5 K while keeping per-chunk memory at a bounded 1 MB. Larger chunks yield diminishing returns because they no longer align with any HDF5 chunk shape.

### Memory footprint

Peak working memory:

- `featureVoxelCounts` — `numFeatures × 8 B` (thousands of features → tens of KB).
- `featureVolumes`, `featureCompensators` — same size (RectGrid only).
- Feature IDs chunk buffer — 1 MB.
- Element sizes chunk buffer — 1 MB (RectGrid only).

All feature-level allocations are O(features), which is inherently small (thousands); chunk buffers are O(1) in dataset size. Billion-voxel volumes run without pressure on RAM.

## Image Geometry Additional Considerations

A typical Image Stack *(an `Image Geometry` that contains 3 dimensions greater than 1)* conceptually consists of a series of 2D images stacked on top of one another to create a 3D object, thus it functions in 3D space as expected. This means it produces **Volumes** and **Equivalent Spherical Diameters**.

Due to the way *Image Geometry* is stored, if you have a `1` in any of the dimensions
it will be turned into a 2D calculation instead. This means **Areas instead of Volumes** and **Equivalent Circular Diameters instead of Equivalent Spherical Diameters**. Furthermore, you must have two dimensions greater than `1` to pass preflight. The reason for this being orientation becomes ambiguous once a second dimension is "empty".

To illustrate why this is think about an image geometry with the dimensions of `1x1x5 (XYZ)`. There is no way to tell whether a `Z * X` or `Z * Y` scaling would be appropriate for the area calculation, given the current information accessible in the algorithm.

### Required Input Sources

- **Feature Ids** -- a per-**Cell** integer label array produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ (02) Small IN100 Full Reconstruction
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic
+ (09) Image Segmentation

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
